#include "playercontrols.h"
#include "synth/synthcontext.h"
#include <QFontMetrics>
#include <QProgressBar>
#include <QAudioOutput>
#include <QGridLayout>
#include <QToolButton>
#include <QSlider>
#include <QLabel>
#include <QStyle>
#include <QTimer>

PlayerControls::PlayerControls(QWidget* parent)
: QWidget(parent), ctx(nullptr), output(nullptr), stream(nullptr)
{
  QGridLayout* layout = new QGridLayout(this);

  playButton = new QToolButton(this);
  playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
  layout->addWidget(playButton, 0, 0);
  QObject::connect(playButton, SIGNAL(clicked()), this, SLOT(togglePlay()));

  stopButton = new QToolButton(this);
  stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
  layout->addWidget(stopButton, 0, 1);
  QObject::connect(stopButton, SIGNAL(clicked()), this, SLOT(stop()));

  seekBar = new QSlider(Qt::Horizontal, this);
  seekBar->setRange(0, 0);
  seekBar->setSingleStep(100);
  seekBar->setPageStep(10000);
  seekBar->setTickInterval(10000);
  seekBar->setTracking(false);
  QObject::connect(seekBar, SIGNAL(valueChanged(int)), this, SLOT(seekTo(int)));
  layout->addWidget(seekBar, 0, 2);

  loadingBar = new QProgressBar(this);
  loadingBar->setRange(0, 0);
  loadingBar->setVisible(false);
  layout->addWidget(loadingBar, 0, 2);

  currentTime = new QLabel("0:00", this);
  currentTime->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  currentTime->setMinimumWidth(QFontMetrics(currentTime->font(), currentTime).boundingRect("000:00").width());
  layout->addWidget(currentTime, 0, 3);

  layout->setColumnStretch(2, 1);
  layout->setSpacing(2);
  layout->setContentsMargins(0, 0, 0, 0);

  resizeEvent(nullptr);
}

#include <QtDebug>
void PlayerControls::setContext(SynthContext* context)
{
  ctx = context;
  if (ctx) {
    seekBar->setRange(0, ctx->maximumTime() * 1000);
  } else {
    seekBar->setRange(0, 0);
    setEnabled(false);
  }
  if (output) {
    delete output;
    output = nullptr;
  }
  stream = nullptr;
  updateState();
}

void PlayerControls::updateState()
{
  seekBar->blockSignals(true);
  int ms = ctx ? (ctx->currentTime() * 1000) : 0;
  if (ms > seekBar->maximum()) {
    seekBar->setMaximum(ms);
  }
  if (seekBar->isSliderDown()) {
    ms = seekBar->sliderPosition();
  } else {
    seekBar->setValue(ms);
  }
  int minutes = ms / 60000;
  int seconds = (ms / 1000) % 60;
  currentTime->setText(QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0')));
  seekBar->blockSignals(false);
  if (output && output->state() == QAudio::ActiveState) {
    playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
  } else {
    playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
  }
}

void PlayerControls::seekTo(int ms)
{
  if (ctx) {
    ctx->seek(ms / 1000.0);
  }
  updateState();
}

void PlayerControls::play()
{
  if (output && output->state() == QAudio::StoppedState) {
    output->deleteLater();
    output = nullptr;
  }
  if (seekBar->value() >= seekBar->maximum()) {
    seekTo(0);
  } else {
    seekTo(seekBar->value());
  }
  if (!output) {
    QAudioFormat format;
    format.setCodec("audio/pcm");
    format.setChannelCount(ctx->outputChannels);
    format.setSampleSize(16);
    format.setSampleRate(ctx->sampleRate);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    output = new QAudioOutput(format, this);
    int bufferSize = format.sampleRate() * format.channelCount() * format.sampleSize() * 100 / 8000;
    output->setBufferSize(bufferSize);
    output->setNotifyInterval(16);
    QObject::connect(output, SIGNAL(stateChanged(QAudio::State)), this, SLOT(stateChanged(QAudio::State)));
    QObject::connect(output, SIGNAL(notify()), this, SLOT(copyBuffer()));
    stream = output->start();
    copyBuffer();
  } else {
    output->resume();
  }
}

void PlayerControls::pause()
{
  if (output) {
    output->suspend();
  }
}

void PlayerControls::stop()
{
  if (output) {
    output->stop();
    output->deleteLater();
    output = nullptr;
  }
  seekBar->setValue(0);
}

void PlayerControls::togglePlay()
{
  if (!output) {
    play();
  } else if (output->state() == QAudio::ActiveState) {
    pause();
  } else {
    play();
  }
}

void PlayerControls::setLoading(bool loading)
{
  loadingBar->setVisible(loading);
  seekBar->setVisible(!loading);
}

void PlayerControls::showEvent(QShowEvent*)
{
  resizeEvent(nullptr);
}

void PlayerControls::resizeEvent(QResizeEvent*)
{
  int h1 = seekBar->sizeHint().height();
  int h2 = loadingBar->sizeHint().height();
  int h3 = playButton->sizeHint().height();
  if (h1 > h2) {
    h1 = h2;
  }
  seekBar->setFixedHeight(h1);
  loadingBar->setFixedHeight(h1);
  if (h1 < h3) {
    h1 = h3;
  }
  setFixedHeight(h1);
}

void PlayerControls::stateChanged(QAudio::State)
{
  updateState();
}

void PlayerControls::copyBuffer()
{
  int needed = output->bytesFree();
  while (needed > 0) {
    int available = buffer.size();
    int toCopy = needed < available ? needed : available;
    int written = 0;
    if (toCopy > 0) {
      written = stream->write(buffer.data(), toCopy);
      buffer.remove(0, written);
      available -= written;
      needed -= written;
    }
    if (available < output->periodSize()) {
      QByteArray readBuffer(output->periodSize(), '\0');
      int read = ctx->fillBuffer(reinterpret_cast<uint8_t*>(readBuffer.data()), readBuffer.size());
      if (read <= 0) {
        if (needed > 0 && written <= 0) {
          output->stop();
          break;
        }
      } else {
        buffer += readBuffer.left(read);
      }
    }
  }
  updateState();
}
