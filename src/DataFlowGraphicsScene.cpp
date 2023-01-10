#include "DataFlowGraphicsScene.hpp"

#include "ConnectionGraphicsObject.hpp"
#include "GraphicsView.hpp"
#include "NodeDelegateModelRegistry.hpp"
#include "NodeGraphicsObject.hpp"
#include "qapplication.h"

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGraphicsSceneMoveEvent>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QWidgetAction>

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QtGlobal>

#include <stdexcept>
#include <utility>


namespace QtNodes
{


DataFlowGraphicsScene::
DataFlowGraphicsScene(DataFlowGraphModel& graphModel,
                      QObject*            parent)
  : BasicGraphicsScene(graphModel, parent)
  , _graphModel(graphModel)
{
  connect(&_graphModel, &DataFlowGraphModel::inPortDataWasSet,
          [this](NodeId const nodeId, PortType const, PortIndex const)
          {
            onNodeUpdated(nodeId);
          });
}


// TODO constructor for an empyt scene?


std::vector<NodeId>
DataFlowGraphicsScene::
selectedNodes() const
{
  QList<QGraphicsItem*> graphicsItems = selectedItems();

  std::vector<NodeId> result;
  result.reserve(graphicsItems.size());

  for (QGraphicsItem* item : graphicsItems)
  {
    auto ngo = qgraphicsitem_cast<NodeGraphicsObject*>(item);

    if (ngo != nullptr)
    {
      result.push_back(ngo->nodeId());
    }
  }

  return result;
}


QMenu*
DataFlowGraphicsScene::
createSceneMenu(QPointF const scenePos)
{
  auto registry = _graphModel.dataModelRegistry();

  QMenu* modelMenuCustom = new QMenu();
  for (auto const& cat : registry->categories())
  {
      QAction *action = new QAction(cat);
      action->setIcon(registry->registeredIcons().value(cat));
      modelMenuCustom->addAction(action);
      connect(action, &QAction::triggered, [=] () {
          NodeId nodeId = this->_graphModel.addNode("Node");

          if (nodeId != InvalidNodeId)
          {
              _graphModel.setNodeData(nodeId,
                                      NodeRole::Position,
                                      scenePos);
          }

          modelMenuCustom->close();

      });
  }

  // QMenu's instance auto-destruction
//  modelMenuCustom->setAttribute(Qt::WA_DeleteOnClose);

  return modelMenuCustom;
}


void
DataFlowGraphicsScene::
save() const
{
  QString fileName =
    QFileDialog::getSaveFileName(nullptr,
                                 tr("Open Flow Scene"),
                                 QDir::homePath(),
                                 tr("Flow Scene Files (*.flow)"));

  if (!fileName.isEmpty())
  {
    if (!fileName.endsWith("flow", Qt::CaseInsensitive))
      fileName += ".flow";

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly))
    {
      file.write(QJsonDocument(_graphModel.save()).toJson());
    }
  }
}


void
DataFlowGraphicsScene::
load()
{
  QString fileName =
    QFileDialog::getOpenFileName(nullptr,
                                 tr("Open Flow Scene"),
                                 QDir::homePath(),
                                 tr("Flow Scene Files (*.flow)"));

  if (!QFileInfo::exists(fileName))
    return;

  QFile file(fileName);

  if (!file.open(QIODevice::ReadOnly))
    return;

  clearScene();

  QByteArray const wholeFile = file.readAll();

  _graphModel.load(QJsonDocument::fromJson(wholeFile).object());

  Q_EMIT sceneLoaded();
}


}
