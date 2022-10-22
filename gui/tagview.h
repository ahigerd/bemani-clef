#ifndef S2W_TAGVIEW_H
#define S2W_TAGVIEW_H

#include <QGroupBox>
#include <QList>
#include <string>
#include "tagmap.h"
class S2WPluginBase;
class QLabel;
class QFormLayout;

class TagView : public QGroupBox
{
Q_OBJECT
public:
  TagView(QWidget* parent = nullptr);

public slots:
  void loadTags(S2WPluginBase* plugin, const std::string& fullPath, const QString& filename);
  void clearTags();

private:
  QFormLayout* layout;
  QList<std::string> tagOrder;
};

#endif
