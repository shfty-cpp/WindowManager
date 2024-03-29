#include "public/TreeModel.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRect>

#include <QDebug>
Q_LOGGING_CATEGORY(treeModel, "app.treeModel")

#include "TreeItem.h"

TreeModel::TreeModel(QObject *parent)
	: QObject(parent)
	, m_autosavePath(QString(QCoreApplication::arguments().at(3)))
	, m_rootItem(nullptr)
{
	load();
}


void TreeModel::load()
{
	qCInfo(treeModel) << "Load";

	bool fileExists = QFileInfo::exists(m_autosavePath) && QFileInfo(m_autosavePath).isFile();
	if(!fileExists)
	{
		saveDefault();
	}

	m_rootItem = new TreeItem(this);

	QFile file(m_autosavePath);
	file.open(QIODevice::ReadOnly);
	QByteArray loadData = file.readAll();
	file.close();

	QJsonDocument loadDocument = QJsonDocument::fromJson(loadData);
	m_rootItem->loadFromJson(loadDocument.object());

	rootItemChanged();
}

void TreeModel::startup()
{
	qCInfo(treeModel) << "Startup";

	m_rootItem->startup();
}

void TreeModel::save()
{
	qCInfo(treeModel) << "Save";

	QJsonDocument jsonDoc(m_rootItem->toJsonObject());
	QFile jsonFile(m_autosavePath);
	jsonFile.open(QIODevice::WriteOnly);
	jsonFile.write(jsonDoc.toJson());
	jsonFile.close();
}

void TreeModel::saveDefault()
{
	qCInfo(treeModel) << "Save Default";

	TreeItem* model = new TreeItem(this);
	model->setObjectName("Monitor");
	model->setProperty("flow", "Horizontal");
	model->setProperty("layout", "Split");

	model->addChild("Desktop", "Horizontal", "Split");

	save();
}
