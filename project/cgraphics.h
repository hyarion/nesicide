#ifndef CGRAPHICS_H
#define CGRAPHICS_H

#include "iprojecttreeviewitem.h"
#include "ixmlserializable.h"

class CGraphics : public IXMLSerializable, public IProjectTreeViewItem
{
public:
    CGraphics();
    virtual ~CGraphics();

    // IXMLSerializable Interface Implementation
    virtual bool serialize(QDomDocument &doc, QDomNode &node);
    virtual bool deserialize(QDomDocument &doc, QDomNode &node);

    // IProjectTreeViewItem Interface Implmentation
    QString caption() const { return QString("Graphics"); }
    virtual void contextMenuEvent(QContextMenuEvent*, QTreeView*) {}
    virtual void openItemEvent(QTabWidget*) {}
    virtual bool onCloseQuery() { return true; }
    virtual void onClose() {}
    virtual int getTabIndex() { return -1; }
    virtual bool isDocumentSaveable() { return false; }
    virtual void onSaveDocument() {}
    virtual bool canChangeName() { return false; }
    virtual bool onNameChanged(QString) { return true; }

};

#endif // CGRAPHICS_H