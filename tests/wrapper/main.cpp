#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>

#include <QOpcUaServer>
#include <QOpcUaBaseObject>
#include <QOpcUaFolderObject>
#include <QOpcUaBaseVariable>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

	QOpcUaServer server;
	auto objsFolder = server.get_objectsFolder();

	auto objBase1   = objsFolder->addBaseObject();
	objBase1->set_displayName("MyObject");
	objBase1->set_description("This is my first object");

	auto varBase1 = objsFolder->addBaseDataVariable();
	varBase1->set_displayName("MyDataVariable");
	varBase1->set_description("This is my first data variable");
	//varBase1->set_value(QVariant::fromValue(QList<int>({ 1, 2, 3 })));
	//varBase1->set_value(QVariant::fromValue(QList<int>()));
	//varBase1->set_value(QVariantList() << 1.1 << 2.2 << 3.3);
	//varBase1->set_value(QStringList() << "1.1" << "2.2" << "3.3");
	//varBase1->set_dataType(QMetaType::ULongLong);
	//varBase1->set_value(QVariantList() << 1 << 4 << 6);
	varBase1->set_value(123);
	//
	//qDebug() << varBase1->get_value();
	//qDebug() << varBase1->get_arrayDimensions();
	
	auto folder1 = objsFolder->addFolderObject();
	folder1->set_displayName("MyFolder");
	folder1->set_description("This is my first folder");

	auto objBaseNested1 = folder1->addBaseObject();
	objBaseNested1->set_displayName("MyObject_Nested");
	objBaseNested1->set_description("This is my first object nested within a folder");
	objBaseNested1->set_browseName({ 1, "My Browse Name 1" });

	auto objBaseNested2 = objBaseNested1->addBaseObject();
	objBaseNested2->set_displayName("MyObject_DoublyNested");
	objBaseNested2->set_description("This is an object nested within another object");
	objBaseNested2->set_browseName({ 1, "Browse Name 2" });

	auto varBase2 = objBaseNested1->addBaseDataVariable();
	varBase2->set_displayName("DataVar2");

	auto folder2 = objBaseNested1->addFolderObject();
	folder2->set_displayName("MyFolder2");

	// [NOTE] blocking, TODO : move to thread
	server.start();

    return a.exec();
}