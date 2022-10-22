#ifndef S2W_MAINWINDOW_H
#define S2W_MAINWINDOW_H

#include <QMainWindow>
#include <QAtomicInteger>
class S2WPluginBase;
class SynthContext;
class TagView;
class PlayerControls;

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
  MainWindow(S2WPluginBase* plugin);

  void openFile(const QString& path, bool autoPlay = false);

private slots:
  void openFile();
  void about();
  void unlockWork();

private:
  bool lockWork();
  void openFile(const QString& path, bool doAcquire, bool autoPlay);

  S2WPluginBase* m_plugin;
  SynthContext* ctx;
  TagView* tagView;
  PlayerControls* controls;
  QAtomicInteger<bool> busy;
  bool m_autoPlay;

  QList<QWidget*> lockWidgets;
  QList<QAction*> lockActions;
};

#endif
