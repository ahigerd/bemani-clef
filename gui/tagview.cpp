#include "tagview.h"
#include "plugin/baseplugin.h"
#include <QFormLayout>
#include <QLabel>
#include <QtDebug>

TagView::TagView(QWidget* parent)
: QGroupBox(parent)
{
  tagOrder << "title" << "artist" << "album" << "albumartist";

  layout = new QFormLayout(this);
  clearTags();
}

void TagView::loadTags(S2WPluginBase* plugin, const std::string& fullPath, const QString& filename)
{
  clearTags();
  setTitle(filename);
  auto stream = plugin->context()->openFile(fullPath);
  TagMap tags = plugin->getTags(fullPath, *stream);
  int sampleRate = plugin->sampleRate(fullPath, *stream);
  double duration = plugin->length(fullPath, *stream);
  int channels = plugin->channels();
  int minutes = duration / 60;
  int seconds = duration - (minutes * 60);
  int ms = (duration - (minutes * 60 + seconds)) * 1000;
  if (ms) {
    layout->addRow(tr("Duration:"), new QLabel(QString("%1:%2.%3").arg(minutes).arg(seconds, 2, 10, QChar('0')).arg(ms, 3, 10, QChar('0'))));
  } else {
    layout->addRow(tr("Duration:"), new QLabel(QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'))));
  }
  layout->addRow(tr("Sample Rate:"), new QLabel(QString::number(sampleRate)));
  if (channels) {
    layout->addRow(tr("Channels:"), new QLabel(QString::number(channels)));
  } else {
    layout->addRow(tr("Channels:"), new QLabel(tr("(unknown)")));
  }
  for (const std::string& tag : tagOrder) {
    auto iter = tags.find(tag);
    if (iter != tags.end()) {
      layout->addRow(QString::fromStdString(iter->first) + ":", new QLabel(QString::fromStdString(iter->second), this));
    }
  }
  for (const auto& iter : tags) {
    if (!tagOrder.contains(iter.first)) {
      layout->addRow(QString::fromStdString(iter.first) + ":", new QLabel(QString::fromStdString(iter.second), this));
      tagOrder << iter.first;
    }
  }
}

void TagView::clearTags()
{
  while (layout->rowCount()) {
    layout->removeRow(layout->rowCount() - 1);
  }
  setTitle(tr("(No File Loaded)"));
}
