# QUaServer

This is a [Qt](https://www.qt.io/) based library that provides a C++ **wrapper** for the [open62541](https://open62541.org/) library, and **abstraction** for the OPC UA Server API.

By *abstraction* it is meant that some of flexibility provided by the original *open62541* server API is sacrificed for ease of use. If more flexibility is required than what *QUaServer* provides, it is highly recommended to use the original *open62541* instead.

The main goal of this library is to provide an object-oriented API that allows quick prototyping for OPC UA servers without having to spend much time in creating complex address space structures.

*QUaServer* is still work in progress, test properly and use precaution before using in production. Please report any issues you encounter in this repository providing a minimum working code example that replicates the issue and a thorough description.

```
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

To test a *QUaServer* based application it is recommended to use the [UA Expert](https://www.unified-automation.com/downloads/opc-ua-clients.html) OPC UA Client.

## Table of Contents  

* [Include](#Include)

How to include QUaServer in your Qt project.

* [Basics](#Basics)

How to create object and variable instances and set their attributes.

* [Methods](#Methods)

How to create methods on the server than can be remotelly called from any client.

* [References](#References)

How to create *non-hierarchical* references between nodes and browse such relations.

* [Types](#Types)

How to create custom object and variable types.

* [Server](#Server)

How to customize the server's properties.

* [Users](#Users)

How to implement access control for the server based on user permissions.

* [Encryption](#Encryption)

How to enable and configure secure encrypted communication between the server and clients.

* [Events](#Events)

How to create and emit custom events.

* [Serialization](#Serialization)

How to serialize and deserialize the server address space to and from disk.

* [Historizing](#Historizing)

How to enable and store historical data and historical events.

* [Alarms](#Alarms)

How to create server alarms and conditions.

---

## Include

This library requires at least `Qt 5.9` or higher and `C++ 11`.

To use *QUaServer*, first a copy of the *open62541* shared library is needed. The [open62541 repo](https://github.com/open62541/open62541) is included in this project as a **git submodule** ([`./depends/open62541.git`](./depends/open62541.git)). So don't forget to clone this repository **recursively**, or run `git submodule update --init --recursive` after cloning this repo.

The *open62541* amalgamation on can be created using the following *QMake* command on the [amalgamation project](./src/amalgamation) included in this repo:

```bash
cd ./src/amalgamation
# Windows
qmake -tp vc amalgamation.pro
msbuild open62541.vcxproj /p:Configuration=Debug
msbuild open62541.vcxproj /p:Configuration=Release
# Linux
qmake amalgamation.pro
make all
```

The [`./depends/open62541.git`](./depends/open62541.git) submodule on this repo (used in the [amalgamation project](./src/amalgamation)), tracks the latest **compatible** *open62541* version, which might not be the most recent version of their master branch. Compatibility of *QUaServer* with the latest version of *open62541* is not always guaranteed.

After compiling the amalgamation, to include *QUaServer* in your project, just include [./src/wrapper/quaserver.pri](./src/wrapper/quaserver.pri) into your Qt project file (`*.pro` file). For example:

```cmake
QT += core
QT -= gui

CONFIG += c++11

TARGET = my_project
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += $$PWD/

SOURCES += main.cpp

include($$PWD/../../src/wrapper/quaserver.pri)
```

### Examples

This library comes with examples in the `./examples` folder, which are explained in detail throughout this document. To build the examples use the [`examples.pro`](./examples.pro) included in the root of this repository:

```bash
# Windows
qmake -r -tp vc examples.pro
msbuild examples.sln /p:Configuration=Debug
# Linux
qmake -r examples.pro
make all
```

---

## Basics

To start using *QUaServer* it is necessary to include the `QUaServer` header as follows:

```c++
#include <QUaServer>
```

To create a server simply create an `QUaServer` instance and call the `start()` method:

```c++
int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	// create server
	QUaServer server;
	// start server
	server.start();

	return a.exec(); 
}
```

Note it is necessary to create a `QCoreApplication` and execute it, because `QUaServer` makes use of [Qt's event loop](https://wiki.qt.io/Threads_Events_QObjects).

By default the *QUaServer* listens on port **4840** which is the [IANA assigned port](https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xhtml?search=4840) for OPC UA applications. To change the listening port, simply pass call the `setPort` method **before** starting the server:

```c++
server.setPort(8080);
```

To start creating OPC *Objects* and *Variables* it is necessary to get the *Objects Folder* of the server and start adding instances to it:

```c++
int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	QUaServer server;

	// get objects folder
	QUaFolderObject * objsFolder = server.objectsFolder();

	// add some instances to the objects folder
	QUaBaseDataVariable * varBaseData = objsFolder->addBaseDataVariable("my_variable");
	QUaProperty         * varProp     = objsFolder->addProperty("my_property");
	QUaBaseObject       * objBase     = objsFolder->addBaseObject("my_object");
	QUaFolderObject     * objFolder   = objsFolder->addFolderObject("my_folder");

	// set values to variables
	varBaseData->setValue(1);
	varProp->setValue("hola");

	server.start();

	return a.exec(); 
}
```

Instances must only be added using the *QUaServer* API, by using the following methods:

* `addProperty` : Adds a `QUaProperty` instance. [*Properties*](https://reference.opcfoundation.org/v104/Core/docs/Part3/4.4.2/) are the **leaves** of the Address Space tree and cannot have other children. They are used to charaterise what its parent represents and their value do not change often. For example, an *engineering unit* or a *brand name*.

* `addBaseDataVariable` : Adds a `QUaBaseDataVariable` instance. [*BaseDataVariables*](https://reference.opcfoundation.org/v104/Core/docs/Part3/5.6.4/) are used to hold data which might change often and can have children (*Objects*, *Properties*, other *BaseDataVariables*). An example is the *current value* of a temperature sensor.

* `addBaseObject` : Adds a `QUaBaseObject` instance. [*BaseObjects*](https://reference.opcfoundation.org/v104/Core/docs/Part3/5.5.1/) can have children and are used to organize other *Objects*, *Properties*, *BaseDataVariables*, etc. The purpose of objects is to **model** a real device. For example a temperature sensor which has *engineering unit* and *brand name* as properties and *current value* as a variable.

* `addFolderObject` : Adds a `QUaFolderObject` instance. [*FolderObjects*](https://reference.opcfoundation.org/v104/Core/docs/Part3/5.5.3/) derive from *BaseObjects* and can do the same, but are typically use to organize a collection of objects. The so called **Objects Folder** is a `QUaFolderObject` instance that always exists on the server to serve as a container for all the user instances.

Once connected to the server, the [address space](https://reference.opcfoundation.org/v104/Core/docs/Part1/6.3.4/) should look something like this:

<p align="center">
  <img src="./res/img/01_basics_02.jpg">
</p>

The string argument passed to these methods defines both the node's initial [`DisplayName`](https://reference.opcfoundation.org/v104/Core/docs/Part3/5.2.5/) and [`BrowseName`](https://reference.opcfoundation.org/v104/Core/docs/Part3/5.2.4/). The `DisplayName` is the name that is displayed to the user by client applications, it should be a human-friendly name. The `BrowseName` is the name that is used programmatically by client applications to find children nodes easily in hierarchical node structures. The `BrowseName` is **inmutable** once a node instance has been created, and must be **unique** with respect to its parent, so one parent node cannot have multiple children with the same `BrowseName`. The `DisplayName` has no restrictions, so it can be changed programmatically.

The `Value` is also set for the variables defined in the example above. The `DataType` of the `Value` is inferred automatically by `QUaServer`, but it can also be set explicitly as it will be shown later.

The `DisplayName`, `BrowseName`, `Value` and `DataType` are [**OPC Attributes**](https://reference.opcfoundation.org/v104/Robotics/docs/3.4.3/). Depending on the type of the instance (*Properties*, *BaseDataVariables*, etc.) it is possible to set different attributes. All OPC instance types derive from the [**Node**](https://reference.opcfoundation.org/v104/Core/docs/Part3/4.3.1/) type. Similarly, in *QUaServer*, all the types derive directly or indirectly from the C++ `QUaNode` abstract class. 

The *QUaServer* API allows to read and write the instances attributes with the following methods:

### For all Types

The *QUaNode* API provides the following methods to access attributes:

```c++
QUaLocalizedText displayName   () const;
void             setDisplayName(const QUaLocalizedText &displayName);

QUaLocalizedText description   () const;
void             setDescription(const QUaLocalizedText &description);

quint32 writeMask     () const;
void    setWriteMask  (const quint32 &writeMask);

QUaNodeId nodeId() const;

QString nodeClass() const;

QUaQualifiedName browseName() const;
```

The `nodeId()` method returns an object containing the [**NodeId**](https://reference.opcfoundation.org/v104/Core/docs/Part3/5.2.2/), which is a unique identifier of the node. This is the only **unique** identifier of a node within a server, because neither the `BrowseName` nor `DisplayName` attributes are unique.

By default a random `NodeId` is assigned automatically when creating a node instance. It is possible to define a custom `NodeId` upon instantiation by passing the string *XML notation* as the *second* argument to the respective method. If the NodeId is invalid or already exists, creating the instance will fail returning `nullptr`. For example:

```c++
QUaProperty * varProp = objsFolder->addProperty("my_property", "ns=1;s=my_prop");
if(!varProp)
{
	qDebug() << "Creating instance failed!";
}
```

To notify changes, the *QUaNode* API provides the following *Qt signals*:

```c++
void displayNameChanged(const QUaLocalizedText &displayName);
void descriptionChanged(const QUaLocalizedText &description);
void writeMaskChanged  (const quint32 &writeMask);
```

Furthermore, the API also notifies when a child is added to an *QUaBaseObject* or *QUaBaseDataVariable* instance:

```c++
void childAdded(QUaNode * childNode);
```

### For Variable Types

Both `QUaBaseDataVariable` and `QUaProperty` derive from the abstract C++ class `QUaBaseVariable` which provides the following methods to access the variable's [**attributes**](https://reference.opcfoundation.org/v104/Core/docs/Part3/5.6.2/):

```c++
QVariant          value() const;
void              setValue(const QVariant &value);

QDateTime         sourceTimestamp() const;
void              setSourceTimestamp(const QDateTime& sourceTimestamp);

QDateTime         serverTimestamp() const;
void              setServerTimestamp(const QDateTime& serverTimestamp);

QUaStatusCode     statusCode() const;
void              setStatusCode(const QUaStatusCode& statusCode);

QMetaType::Type   dataType() const;
void              setDataType(const QMetaType::Type &dataType);

qint32            valueRank() const;
QVector<quint32>  arrayDimensions() const; 

quint8            accessLevel() const;
void              setAccessLevel(const quint8 &accessLevel);

double            minimumSamplingInterval() const;
void              setMinimumSamplingInterval(const double &minimumSamplingInterval);

bool              historizing() const;
```

The `value`, `sourceTimestamp`, `serverTimestamp` and `statusCode` can be set in a single call by using the `setValue` overload:

```c++
void setValue(
	const QVariant &value, 
	const QUaStatusCode   &statusCode,
	const QDateTime       &sourceTimestamp,
	const QDateTime       &serverTimestamp
);
```

The `setDataType()` can be used to *force* a data type on the variable value. The following [Qt types](https://doc.qt.io/qt-5/qmetatype.html#Type-enum) are supported, as well as their `QList<T>` and `QVector<T>` types:

```c++
QMetaType::Bool
QMetaType::Char
QMetaType::SChar
QMetaType::UChar
QMetaType::Short
QMetaType::UShort
QMetaType::Int
QMetaType::UInt
QMetaType::Long
QMetaType::LongLong
QMetaType::ULong
QMetaType::ULongLong
QMetaType::Float
QMetaType::Double
QMetaType::QString
QMetaType::QDateTime
QMetaType::QUuid
QMetaType::QByteArray
```

The `setAccessLevel()` method allows to set a bit mask to define the overall variable read and write access. Nevertheless, the `QUaBaseVariable` API provides a couple of helper methods that allow to define the access more easily without needing to deal with bit masks:

```c++
// Default : read access true
bool readAccess() const;
void setReadAccess(const bool &readAccess);
// Default : write access false
bool writeAccess() const;
void setWriteAccess(const bool &writeAccess);
```

Using such methods we could set a variable to be **writable**, for example:

```c++
QUaBaseDataVariable * varBaseData = objsFolder->addBaseDataVariable();
varBaseData->setWriteAccess(true);
```

When a variable is written from a client, on the server notifications are provided by the `void QUaBaseVariable::valueChanged(const QVariant &value, const bool &networkChange)` Qt signal.

```c++
QObject::connect(varBaseData, &QUaBaseDataVariable::valueChanged, [](const QVariant &value, const bool &networkChange) {
	qDebug() << "New value :" << value << (networkChange ? "(Network)" : "(Server Logic)");
});
```

The `networkChange` argument specifies if the value was changed though the network by an OPC client or if the value change was performed programmatically by internal the server logic.

### For Object Types

The API provides the following methods to access attributes:

```c++
quint8 eventNotifier() const;
void   setEventNotifier(const quint8 &eventNotifier);

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
bool subscribeToEvents() const;
void setSubscribeToEvents(const bool& subscribeToEvents);
#endif // UA_ENABLE_SUBSCRIPTIONS_EVENTS
```

The usage of these methods is described in detail in the *Events* section.

### Basics Example

Build and test the basics example in [./examples/01_basics](./examples/01_basics/main.cpp) to learn more.

---

## Methods

In OPC UA, *BaseObjects* instances can have methods. To support this, the *QUaBaseObject* API has the `addMethod()` method which allows to define a name for the method and a callback.

Since the *Objects Folder* is an instance of `QUaBaseObject`, it is possible to add methods to it directly, for example:

```c++
int addNumbers(int x, int y)
{
	return x + y;
}

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	QUaServer server;

	QUaFolderObject * objsFolder = server.objectsFolder();

	// add a method using callback function
	objsFolder->addMethod("addNumbers", &addNumbers);

	server.start();

	return a.exec(); 
}
``` 

Which can be remotely executed using a client.

<p align="center">
  <img src="./res/img/02_methods_01.jpg">
</p>

Note that the *QUaServer* library automatically deduces the arguments and return types. But only the types supported by the `setDataType()` method (see the *Basics* section) are supported by the `addMethod()` API.

A more *flexible* way of adding methods is by using **C++ Lambdas**:

```c++
objsFolder->addMethod("increaseNumber", [](double input) {
	double increment = 0.1;
	return input + increment;
});
```

<p align="center">
  <img src="./res/img/02_methods_02.jpg">
</p>

Using the *Lambda Capture* it is possible to change *Objects* or *Variables*:

```c++
auto varNumber = objsFolder->addBaseDataVariable();
varNumber->setDisplayName("Number");
varNumber->setValue(0.0);
varNumber->setDataType(QMetaType::Double);

objsFolder->addMethod("incrementNumberBy", [&varNumber](double increment) {
	double currentValue = varNumber->value().toDouble();
	double newValue = currentValue + increment;
	varNumber->setValue(newValue);
	return true;
});
```

<p align="center">
  <img src="./res/img/02_methods_03.jpg">
</p>

Using methods we can even **delete** *Objects* or *Variables*:

```c++
objsFolder->addMethod("deleteNumber", [&varNumber]() {
	if (!varNumber)
	{
		return;
	}
	delete varNumber;
	varNumber = nullptr;
});
```

<p align="center">
  <img src="./res/img/02_methods_04.jpg">
</p>

### Methods Example

Build and test the methods example in [./examples/02_methods](./examples/02_methods/main.cpp) to learn more.

---

## References

OPC UA supports the concept of *References* to create relations between *Nodes*. References are categorised in *HierarchicalReferences* and *NonHierarchicalReferences*. The *HierarchicalReferences* are the ones used by most OPC Clients to display the instances tree in their graphical user interfaces.

When adding an instance using the *QUaServer* API, the library creates the required *HierarchicalReference* type necessary to display the new instance in the instances tree (it uses the *HasComponent*, *HasProperty* or *Organizes* reference types accordingly).

The *QUaServer* API also allows to create **custom** *NonHierarchicalReferences* that can be used to create custom relations between instances. For example, having a temperature sensor and then define a supplier for that sensor:

```c++
// create sensor
QUaBaseObject * objSensor1 = objsFolder->addBaseObject("TempSensor1");
// create supplier
QUaBaseObject * objSupl1 = objsFolder->addBaseObject("Mouser");
// create reference
server.registerReference({ "Supplies", "IsSuppliedBy" });
objSupl1->addReference({ "Supplies", "IsSuppliedBy" }, objSensor1);
```

The `registerReference()` method has to be called in order to *register* the new reference type as a *subtype* of the *NonHierarchicalReferences*. If the reference type is not registered before its first use, it is registered automatically on first use. 

The registered reference can be observed when the server is running by browsing to `/Root/Types/ReferenceTypes/NonHierarchicalReferences`. There should be a new entry corresponding to the custom reference.

<p align="center">
  <img src="./res/img/03_references_01.jpg">
</p>

The references for the supplier object should list the *Supplies* reference:

<p align="center">
  <img src="./res/img/03_references_02.jpg">
</p>

The references for the sensor object should list the *IsSuppliedBy* reference:

<p align="center">
  <img src="./res/img/03_references_03.jpg">
</p>

The `registerReference()` actually receives a `QUaReference` instance as an argument, which is defined as:

```c++
struct QUaReference
{
	QString strForwardName;
	QString strInverseName;
};
```

Both **forward** and **reverse** names of the reference have to be defined in order to create the reference. In the example, `Supplies` is the *forward* name, and `IsSuppliedBy` is the reverse name. When adding a reference, by default, it is added in *forward* mode. This can be changed by adding a third argument to the `addReference()` method which is `true` by default to indicate it is *forward*, `false` to indicate it is *reverse*. 

```c++
// objSupl1 "Supplies" objSensor1
objSupl1->addReference({ "Supplies", "IsSuppliedBy" }, objSensor1, true);
// objSensor2 "IsSuppliedBy" objSupl1
objSensor2->addReference({ "Supplies", "IsSuppliedBy" }, objSupl1, false);
```

In the example above, both sensors are supplied by the same supplier:

<p align="center">
  <img src="./res/img/03_references_04.jpg">
</p>

Programmatically, references can be added, removed and browsed using the following *QUaNode* API methods:

```c++
void addReference(const QUaReference &ref, const QUaNode * nodeTarget, const bool &isForward = true);

void removeReference(const QUaReference &ref, const QUaNode * nodeTarget, const bool &isForward = true);

template<typename T>
QList<T*>       findReferences(const QUaReference &ref, const bool &isForward = true);
// specialization
QList<QUaNode*> findReferences(const QUaReference &ref, const bool &isForward = true);
```

For example, to list all the sensors that are supplied by the supplier:

```c++
qDebug() << "Supplier" << objSupl1->displayName() << "supplies :";
auto listSensors = objSupl1->findReferences<QUaBaseObject>({ "Supplies", "IsSuppliedBy" });
for (int i = 0; i < listSensors.count(); i++)
{
	qDebug() << listSensors.at(i)->displayName();
}
```

And to list the supplier of a sensor:

```c++
qDebug() << objSensor1->displayName() << "supplier is: :";
auto listSuppliers = objSensor1->findReferences<QUaBaseObject>({ "Supplies", "IsSuppliedBy" }, false);
qDebug() << listSuppliers.first()->displayName();
```

Note that when a *QUaNode* derived instance is deleted, all its references are removed.

To notify when a reference has been added or removed the *QUaNode* API has the following *Qt signals*:

```c++
void referenceAdded  (const QUaReference & ref, QUaNode * nodeTarget, const bool &isForward);
void referenceRemoved(const QUaReference & ref, QUaNode * nodeTarget, const bool &isForward);
```

### References Example

Build and test the methods example in [./examples/03_references](./examples/03_references/main.cpp) to learn more.

---

## Types

OPC types can be extended by subtyping *BaseObjects* or *BaseDataVariables* (*Properties* cannot be subtyped). Using the *QUaServer* library, a new *BaseObject* subtype can be created by deriving from `QUaBaseObject`. Similarly, a new *BaseDataVariable* subtype can be created by deriving from `QUaBaseDataVariable`.

Subtyping is very useful to **reuse** code. For example, if multiple temperature sensors are to be exposed through the OPC UA Server, it might be worth creating a type for it. Start by sub-classing `QUaBaseObject` as follows:

In `temperaturesensor.h` :

```c++
#include <QUaBaseObject>

class TemperatureSensor : public QUaBaseObject
{
	Q_OBJECT

public:
	Q_INVOKABLE explicit TemperatureSensor(QUaServer *server);
	
};
```

In `temperaturesensor.cpp` :

```c++
#include "temperaturesensor.h"

TemperatureSensor::TemperatureSensor(QUaServer *server)
	: QUaBaseObject(server)
{
	
}
```

There are 3 **important requirements** when creating subtypes:

* Inherit from either *QUaBaseObject* or *QUaBaseDataVariable* (which in turn inherit indirectly from *QObject*). The `Q_OBJECT` macro must be set.

* Create a public constructor that receives a `QUaServer` pointer as an argument. Add the `Q_INVOKABLE` macro to such constructor.

* In the constructor implementation call the parent constructor (*QUaBaseObject*, *QUaBaseDataVariable* or derived parent constructor accordingly).

Once all this is met, elsewhere in the code it is necessary to register the new type in the server using the `registerType<T>()` method. If not registered, then when creating an instance of the new type, the type will be registered automatically by the library.

An instance of the new type is created using the `addChild<T>()` method:

```c++
#include "temperaturesensor.h"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	QUaServer server;

	QUaFolderObject * objsFolder = server.objectsFolder();

	// register new type
	server.registerType<TemperatureSensor>();

	// create new type instance
	auto sensor1 = objsFolder->addChild<TemperatureSensor>("Sensor1");

	server.start();

	return a.exec(); 
}
``` 

If the new type was registered correctly, it can be observed by browsing to `/Root/Types/ObjectTypes/BaseObjectType`. There should be a new entry corresponding to the custom type.

<p align="center">
  <img src="./res/img/04_types_01.jpg">
</p>

Note that the new *TemperatureSensor* type has a `TypeDefinitionOf` reference to the *Sensor1* instance. And the *Sensor1* instance has a `HasTypeDefinition` to the *TemperatureSensor* type.

Adding **child** *Variables*, *Properties* and potentially other *Objects* to the *TemperatureSensor* type is achieved through the [Qt Property System](https://doc.qt.io/qt-5/properties.html). 

Use the `Q_PROPERTY` macro to add pointers to types of desired children and the library will automatically instantiate the children once an specific instance of the *TemperatureSensor* type is created.

```c++
#include <QUaBaseObject>
#include <QUaBaseDataVariable>
#include <QUaProperty>

class TemperatureSensor : public QUaBaseObject
{
	Q_OBJECT
	// properties
	Q_PROPERTY(QUaProperty * model READ model)
	Q_PROPERTY(QUaProperty * brand READ brand)
	Q_PROPERTY(QUaProperty * units READ units)
	// variables
	Q_PROPERTY(QUaBaseDataVariable * status       READ status      )
	Q_PROPERTY(QUaBaseDataVariable * currentValue READ currentValue)
public:
	Q_INVOKABLE explicit TemperatureSensor(QUaServer *server);

	QUaProperty * model();
	QUaProperty * brand();
	QUaProperty * units();

	QUaBaseDataVariable * status      ();
	QUaBaseDataVariable * currentValue();
};
```

The *QUaServer* library automatically adds the C++ instances as [QObject children](https://doc.qt.io/qt-5/objecttrees.html) of the *TemperatureSensor* instance and assigns them the `Q_PROPERTY` name as their [QObject name](https://doc.qt.io/Qt-5/qobject.html#objectName-prop). Therefore it is possible retrieve the C++ children using the [`findChild` method](https://doc.qt.io/Qt-5/qobject.html#findChild).

```c++
TemperatureSensor::TemperatureSensor(QUaServer *server)
	: QUaBaseObject(server)
{
	// set defaults
	model()->setValue("TM35");
	brand()->setValue("Texas Instruments");
	units()->setValue("C");
	status()->setValue("Off");
	currentValue()->setValue(0.0);
	currentValue()->setDataType(QMetaType::Double);
}

QUaProperty * TemperatureSensor::model()
{
	return this->browseChild<QUaProperty>("model");
}

QUaProperty * TemperatureSensor::brand()
{
	return this->browseChild<QUaProperty>("brand");
}

QUaProperty * TemperatureSensor::units()
{
	return this->browseChild<QUaProperty>("units");
}

QUaBaseDataVariable * TemperatureSensor::status()
{
	return this->browseChild<QUaBaseDataVariable>("status");
}

QUaBaseDataVariable * TemperatureSensor::currentValue()
{
	return this->browseChild<QUaBaseDataVariable>("currentValue");
}
```

Be **careful** when using the `browseChild` to provide the correct `BrowseName`, otherwise a null reference can be returned from any of the getter methods.

Note that in the *TemperatureSensor* constructor it is possible to already make use of the children instances and define some default values for them.

Now it is possible to create any number of *TemperatureSensor* instances and their children will be created and attached to them automatically.

```c++
auto sensor1 = objsFolder->addChild<TemperatureSensor>("Sensor1");
auto sensor2 = objsFolder->addChild<TemperatureSensor>("Sensor2");
auto sensor3 = objsFolder->addChild<TemperatureSensor>("Sensor3");
```

<p align="center">
  <img src="./res/img/04_types_02.jpg">
</p>

Any `Q_PROPERTY` added to the *TemperatureSensor* declaration that **inherits** `QUaProperty`, `QUaBaseDataVariable` or `QUaBaseObject` will be exposed through OPC UA. Else the `Q_PROPERTY` will be created in the C++ instance but not exposed through OPC UA.

To add **methods** to a subtype, the `Q_INVOKABLE` macro can be used. The limitations are than only [up to 10 arguments can used](https://doc.qt.io/qt-5/qmetamethod.html#invoke) and the argument types can only be the same supported by the `setDataType()` method (see the *Basics* section).

```c++
class TemperatureSensor : public QUaBaseObject
{
	Q_OBJECT
	
	// properties, variables, objects ...

public:
	Q_INVOKABLE explicit TemperatureSensor(QUaServer *server);

	// properties, variables, objects ...

	Q_INVOKABLE void turnOn();
	Q_INVOKABLE void turnOff();
};
```

The implementation is like a normal C++ class method:

```c++
void TemperatureSensor::turnOn()
{
	status()->setValue("On");
}

void TemperatureSensor::turnOff()
{
	status()->setValue("Off");
}
```

If the `Q_INVOKABLE` macro is not used, then the method is simply not exposed through OPC UA.

<p align="center">
  <img src="./res/img/04_types_03.jpg">
</p>

One final perk of creating subtypes is the possiblity of creating custom enumerators which can be used as data types for variables. This is done using the `Q_ENUM` macro:

```c++
class TemperatureSensor : public QUaBaseObject
{
	Q_OBJECT
	
	// properties, variables, objects ...

public:
	Q_INVOKABLE explicit TemperatureSensor(QUaServer *server);

	// properties, variables, objects ...
	// methods ...

	enum Units
	{
		C = 0,
		F = 1
	};
	Q_ENUM(Units)

};
```

Then using the enumerator to set the value of a *Variable*:

```c++
TemperatureSensor::TemperatureSensor(QUaServer *server)
	: QUaBaseObject(server)
{
	// set defaults ...
	// use enum as type
	units()->setDataTypeEnum(QMetaEnum::fromType<TemperatureSensor::Units>());
	units()->setValue(Units::C);
}
```

Then any client has knowledge of the enum options.

### Types Example

Build and test the methods example in [./examples/04_types](./examples/04_types/main.cpp) to learn more.

---

## Server

The `QUaServer` class constructor not only allows to set a custom port to run the server (see the *Basics* section), but also to set an SSL **certificate** so that clients can **validate** the server. The `QUaServer` instance also contains methods that allow to customise the server **description** published through OPC UA.

See the [validation document](./VALIDATION.md) for more details on how validation works.

### Create Certificates

Make sure `openssl` is installed and follow the next commands to create the certificate (on Windows use [MSys2](https://www.msys2.org/), on Linux just use the command line).

The first step is to create a Certificate Authority (CA). The CA will take the role of a system integrator comissioned with installing OPC Servers in a plant. The CA will have to:

* Create its own *public* and *private* key pair.

* Create its own **self-signed** *certificate*.

* Create its own *Certificate Revocation List (CRL)*.

Keys can be created and transformed into various formats. Ultimately, most OPC UA applications make use of the [DER format](https://wiki.openssl.org/index.php/DER). 

```bash
# Create directory to store CA's files
mkdir ca
# Create CA key
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out ca/ca.key
# Create self-signed CA cert
openssl req -new -x509 -days 3600 -key ca/ca.key -subj "/CN=juangburgos CA/O=juangburgos Organization" -out ca/ca.crt
# Convert cert to der format
openssl x509 -in ca/ca.crt -inform pem -out ca/ca.crt.der -outform der
# Create cert revocation list CRL file
# NOTE : might need to create in relative path
#        - File './demoCA/index.txt' (Empty)
#        - File './demoCA/crlnumber' with contents '1000'
openssl ca -crldays 3600 -keyfile ca/ca.key -cert ca/ca.crt -gencrl -out ca/ca.crl
# Convert CRL to der format
openssl crl -in ca/ca.crl -inform pem -out ca/ca.der.crl -outform der
```

The next steps must be applied for each server the system integrator wants to install.

* Create its own *public* and *private* key pair.

* Create an `exts.txt` which contain the *certificate extensions* required by the OPC UA standard.

* Create its own **unsigned** certificate, and with it a *certificate sign request*.

* Give the *certificate sign request* to the CA to sign it.

The `exts.txt` should be as follows:

```
[v3_ca]
subjectAltName=DNS:localhost,DNS:ppic09,IP:127.0.0.1,IP:192.168.1.18,URI:urn:unconfigured:application
basicConstraints=CA:TRUE
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid,issuer
keyUsage=digitalSignature,keyEncipherment
extendedKeyUsage=serverAuth,clientAuth,codeSigning
```

The `subjectAltName` must contains all the URLs that will be used to connect to the server. In the example above, clients might connect to the localhost (`127.0.0.1`) or through the Windows network, using the Windows PC name (`ppic09`), or through the local network (`192.168.1.18`).

```bash
# Create directory to store server's files
mkdir server
# Create server key
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out server/server.key
# Convert server key to der format
openssl rsa -in server/server.key -inform pem -out server/server.key.der -outform der
# Create server cert sign request
openssl req -new -sha256 \
-key server/server.key \
-subj "/C=ES/ST=MAD/O=MyServer/CN=localhost" \
-out server/server.csr
```

The CA must now sign the server's *certificate sign request* to create the *signed certificate*, appending also the required *certificate extensions* (`exts.txt`).

```bash
# Sign cert sign request (NOTE: must provide exts.txt)
openssl x509 -days 3600 -req \
-in server/server.csr \
-extensions v3_ca \
-extfile server/exts.txt \
-CAcreateserial -CA ca/ca.crt -CAkey ca/ca.key \
-out server/server.crt
# Convert cert to der format
openssl x509 -in server/server.crt -inform pem -out server/server.crt.der -outform der
```

### Use Certificates

First the CA's certificate and CRL must be copied to the client's software. 

In the case of *UA Expert*, in the user interface go to `Settings -> Manage Certificates...`. Then click the `Open Certificate Location`, which opens the file epxlorer to a location similar to:

```
$SOME_PATH/unifiedautomation/uaexpert/PKI/trusted/certs
```

The CA's certificate must be copied to this path:

```bash
cp ca/ca.crt.der $SOME_PATH/unifiedautomation/uaexpert/PKI/trusted/certs/ca.crt.der
```

Going one directory up, then in `crl` is where the CRL must be copied to:

```bash
cp ca/ca.der.crl $SOME_PATH/unifiedautomation/uaexpert/PKI/trusted/crl/ca.der.crl
```

Now the *server certificate* must be copied next to the *QUaServer* application:

```bash
cp server/server.crt.der $SERVER_PATH/server.crt.der
```

And in the C++ code the server's certificate contents need to be passed to the `setCertificate` method **before** starting the server:

```c++
#include <QCoreApplication>
#include <QDebug>
#include <QFile>

#include <QUaServer>

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	QUaServer server;

	// Load server certificate
	QFile certServer;
	certServer.setFileName("server.crt.der");
	Q_ASSERT(certServer.exists());
	certServer.open(QIODevice::ReadOnly);
	server.setCertificate(certServer.readAll());
	certServer.close();

	server.start();

	return a.exec(); 
}
```

Now the client is able to validate the server before connecting to it.

Note that eventhough validation required creating and managing cryptographic keys, the communications are yet **not encrypted**. The files generated in this section are used in the *Encryption* section to actually encrypt communications.

### Server Description

The `QUaServer` instance also contains methods to add custom server description:

```c++
// Add server description
server.setApplicationName ("my_app");
server.setApplicationUri  ("urn:juangburgos:my_app");
server.setProductName     ("my_product");
server.setProductUri      ("juangburgos.com");
server.setManufacturerName("My Company Inc.");
server.setSoftwareVersion ("6.6.6-master");
server.setBuildNumber     ("gvfsed43fs");
```

This methods should be called **before** starting the server, else the changes won't be visible until the server is restarted.

This information is then made available to the clients through the *Server Object* that can be found by browsing to `/Root/Objects/Server/ServerStatus/BuildInfo`

<p align="center">
  <img src="./res/img/05_server_01.jpg">
</p>

### Server Example

Build and test the server example in [./examples/05_server](./examples/05_server/main.cpp) to learn more.

Some test certificates are included for convenience in [./examples/05_server/ca_files](./examples/05_server/ca_files). **Do not use them in production**, just for testing purposes.

---

## Users

By default, `QUaServer` instances allow *anonymous* login. To disable it use the `setAnonymousLoginAllowed()` method as follows:

```c++
server.setAnonymousLoginAllowed(false);
```

But now it is necessary to create at least one user account to access the server. This can be done using the `addUser()` method:

```c++
server.addUser("juan", "pass123");
server.addUser("john", "qwerty");
```

The first argument is the **username** and the second is the **password**.

Now when trying to connect to the server application without credentials, an error might appear in the client's log:

```
Error 'BadIdentityTokenInvalid' was returned during ActivateSession
```

To connect to the server it is necessary now to provide credentials. For example, with the *UA Expert* client right click the server and select `Properties ...`:

<p align="center">
  <img src="./res/img/06_users_01.jpg">
</p>

Then in *Authentication Settings* select *Username Password* and introduce the username, and click `OK`:

<p align="center">
  <img src="./res/img/06_users_02.jpg">
</p>

Now when connecting, the password will be requested. It is likely that the client will issue a warning:

<p align="center">
  <img src="./res/img/06_users_03.jpg">
</p>

The reason is that communications are **not yet encrypted**, therefore the usermame and password will be sent in **plain text**. So any application that monitors the network (such as [Wireshark](https://www.wireshark.org/)) can read such messages and read the login credentials.

For the moment ignore the warning by clicking `Ignore` to connect to the server. In the *Encryption* section it is detailed how to encrypt communications such that the login credentials are protected from [eavesdropping](https://en.wikipedia.org/wiki/Eavesdropping) applications.

Having a list of login credentials does not only limit access to the server, but is possible to limit access to individual resources in a **per-user** basis using the `setUserAccessLevelCallback()` available in the `QUaNode` API:

```c++
// Create some varibles
QUaFolderObject * objsFolder = server.objectsFolder();
// NOTE : the variables need to be overall writable
//        user-level access is defined later
auto var1 = objsFolder->addProperty("var1");
var1->setWriteAccess(true);
var1->setValue(123);
auto var2 = objsFolder->addProperty("var2");
var2->setWriteAccess(true);
var2->setValue(1.23);

// Give access control to individual variables
var1->setUserAccessLevelCallback([](const QString &strUserName) {
	QUaAccessLevel access;
	// Read Access to all
	access.bits.bRead = true;
	// Write Access only to juan
	if (strUserName.compare("juan", Qt::CaseSensitive) == 0)
	{
		access.bits.bWrite = true;
	}
	else
	{
		access.bits.bWrite = false;
	}
	return access;
});

var2->setUserAccessLevelCallback([](const QString &strUserName) {
	QUaAccessLevel access;
	// Read Access to all
	access.bits.bRead = true;
	// Write Access only to john
	if (strUserName.compare("john", Qt::CaseSensitive) == 0)
	{
		access.bits.bWrite = true;
	}
	else
	{
		access.bits.bWrite = false;
	}
	return access;
});
```

Note the example above uses *C++ Lambdas*, but traditional callbacks can be used.

Now if *John* tries to write `var1`, he might get a client log error like:

```
Write to node 'NS0|Numeric|762789430' failed [ret = BadUserAccessDenied]
```

The user-level access control is implemeted in a **cascading** fashion, meaning that if a variable does not have an specific *UserAccessLevelCallback* defined, then it looks if the parent has one and so on. If no node has a *UserAccessLevelCallback* defined then all access is granted. For example:

```c++
auto * obj = objsFolder->addBaseObject("obj");

auto * subobj = obj->addBaseObject("subobj");

auto subsubvar = subobj->addProperty("subsubvar");
subsubvar->setWriteAccess(true);
subsubvar->setValue("hola");

// Define access on top level object, 
// since no specific access is defined on 'subsubvar',
// it inherits the grandparent's
obj->setUserAccessLevelCallback([](const QString &strUserName){
	QUaAccessLevel access;
	// Read Access to all
	access.bits.bRead = true;
	// Write Access only to juan
	if (strUserName.compare("juan", Qt::CaseSensitive) == 0)
	{
		access.bits.bWrite = true;
	}
	else
	{
		access.bits.bWrite = false;
	}
	return access;
});
```

When creating *custom types* it is possible to define a *default* custom access level by *reimplementing* the `userAccessLevel()` virtual method. For example:

In `customvar.h`:

```c++
#include <QUaBaseDataVariable>
#include <QUaProperty>

class CustomVar : public QUaBaseDataVariable
{
	Q_OBJECT

	Q_PROPERTY(QUaProperty         * myProp READ myProp)
	Q_PROPERTY(QUaBaseDataVariable * varFoo READ varFoo)
	Q_PROPERTY(QUaBaseDataVariable * varBar READ varBar)

public:
	Q_INVOKABLE explicit CustomVar(QUaServer *server);

	QUaProperty         * myProp();
	QUaBaseDataVariable * varFoo();
	QUaBaseDataVariable * varBar();

	// Reimplement virtual method to define default user access
	// for all instances of this type
	QUaAccessLevel userAccessLevel(const QString &strUserName) override;
};
```

In `customvar.cpp`:

```c++
#include "customvar.h"

CustomVar::CustomVar(QUaServer *server)
	: QUaBaseDataVariable(server)
{
	this->myProp()->setValue("xxx");
	this->varFoo()->setValue(true);
	this->varBar()->setValue(69);
	this->myProp()->setWriteAccess(true);
	this->varFoo()->setWriteAccess(true);
	this->varBar()->setWriteAccess(true);
}

QUaProperty * CustomVar::myProp()
{
	return this->findChild<QUaProperty*>("myProp");	
}

QUaBaseDataVariable * CustomVar::varFoo()
{
	return this->findChild<QUaBaseDataVariable*>("varFoo");
}

QUaBaseDataVariable * CustomVar::varBar()
{
	return this->findChild<QUaBaseDataVariable*>("varBar");
}

QUaAccessLevel CustomVar::userAccessLevel(const QString & strUserName)
{
	QUaAccessLevel access;
	// Read Access to all
	access.bits.bRead = true;
	// Write Access only to john
	if (strUserName.compare("john", Qt::CaseSensitive) == 0)
	{
		access.bits.bWrite = true;
	}
	else
	{
		access.bits.bWrite = false;
	}
	return access;
}
```

So any instance of `CustomVar` will use the reimplemented method by default, *unless* there exists a more **specific** callback:

```c++
QUaAccessLevel juanCanWrite(const QString &strUserName) 
{
	QUaAccessLevel access;
	// Read Access to all
	access.bits.bRead = true;
	// Write Access only to juan
	if (strUserName.compare("juan", Qt::CaseSensitive) == 0)
	{
		access.bits.bWrite = true;
	}
	else
	{
		access.bits.bWrite = false;
	}
	return access;
}

// ...

auto custom1 = objsFolder->addChild<CustomVar>("custom1");
auto custom2 = objsFolder->addChild<CustomVar>("custom2");

// Set specific callbacks
custom1->varFoo()->setUserAccessLevelCallback(&juanCanWrite);
custom2->setUserAccessLevelCallback(&juanCanWrite);
```

In the example above, all children variables (`myProp`, `varFoo` and `varBar`) inherit the reimplemented access level defined in the `CustomVar` class, which allows only *john* to write. But, the `varFoo` child of the `custom1` instance has an specific callback that will overrule the parent's permission.

The instance `custom2` also inherits by default the reimplemented access level defined in the `CustomVar` class, but the specific callback overrules the inherited permission.

### Users Example

Build and test the server example in [./examples/06_users](./examples/06_users/main.cpp) to learn more.

---

## Encryption

In this section the *QUaServer* library is configured to encrypt communications. Before continuing, make sure to go through the *Server* section in detail and generate all the required certificates and keys.

To support encryption, it is necessary to add the [mbedtls library](https://github.com/ARMmbed/mbedtls) to the project's dependencies. A copy of a compatible version of the `mbedtls` library is included in this repo as a *git cubmodule* in [`./depends/mbedtls.git`](./depends/mbedtls.git).

The `mbedtls` library is built automatically when compiling the [amalgamation project](./src/amalgamation), by passing the `ua_encryption` option as follows:


```bash
cd ./src/amalgamation
# Windows
qmake "CONFIG+=ua_encryption" -tp vc amalgamation.pro
msbuild open62541.vcxproj
# Linux
qmake "CONFIG+=ua_encryption" amalgamation.pro
make all
```

To compile the examples, run `qmake` again over your project to load the new configuration. For example, to update the examples in this repo run:

```bash
# Windows
qmake "CONFIG+=ua_encryption" -r -tp vc examples.pro
msbuild examples.sln
# Linux
qmake "CONFIG+=ua_encryption" -r examples.pro
make all
```

After running `qmake` it is often necessary to **rebuild** the complete project to avoid *missing symbols* errors.

Now copy the server's **certificate** and **private key** to the path where your binary is (`server.crt.der` and `server.key.der` created in the *Server* section).

Finally load the *certificate* and *private key* in the C++ code and pass them to the `QUaServer` constructor:

```c++
#include <QCoreApplication>
#include <QDebug>
#include <QFile>

#include <QUaServer>

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	// Load server certificate
	QFile certServer;
	certServer.setFileName("server.crt.der");
	Q_ASSERT(certServer.exists());
	certServer.open(QIODevice::ReadOnly);

	// Load server private key
	QFile privServer;
	privServer.setFileName("server.key.der");
	Q_ASSERT(privServer.exists());
	privServer.open(QIODevice::ReadOnly);

	// Instantiate server by passing certificate and key
	QUaServer server;
	server.setCertificate(certServer.readAll());
	server.setPrivateKey (privServer.readAll());

	certServer.close();
	privServer.close();

	server.start();

	return a.exec(); 
}
```

Now when the client browses for the server, there should be a new option to connect using **Sign & Encrypt** which encrypts the communications between clients and the server.

<p align="center">
  <img src="./res/img/07_encryption_02.jpg">
</p>

### Encryption Example

Build and test the encryption example in [./examples/07_encryption](./examples/07_encryption/main.cpp) to learn more.

---

## Events

To use events, it is necessary to create a new amalgamation from the *open62541* source code that supports events. This can be done by building it with the following commands:

```bash
cd ./depends/open62541.git
mkdir build; cd build
# Adjust your Cmake generator accordingly
cmake -DUA_ENABLE_AMALGAMATION=ON -DUA_NAMESPACE_ZERO=FULL -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON .. -G "Visual Studio 15 2017 Win64"
```

* The `-DUA_NAMESPACE_ZERO=FULL` option is needed because by default *open62541* does not include the complete address space of the OPC UA standard in order to reduce binary size. But to support events, it is actually necessary to have the `FULL` address space available in the server application.

* The `-DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON` is the flag that enables events.

Note that the amalgamation files are now considerably larger because now they contain the full default OPC UA address space.

Now build the library using the Qt project included in this repo:

```bash
cd ./src/amalgamation
# Windows
qmake "CONFIG+=ua_events" -tp vc amalgamation.pro
msbuild open62541.vcxproj
# Linux
qmake "CONFIG+=ua_events" amalgamation.pro
make all
```

To update the examples to support events:

```bash
# Windows
qmake "CONFIG+=ua_events" -r -tp vc examples.pro
msbuild examples.sln
# Linux
qmake "CONFIG+=ua_events" -r examples.pro
make all
```

After running `qmake` it is often necessary to **rebuild** the application to avoid *missing symbols* errors.

The building process above is similar than the one described in the encryption section. To enable both events and encryption the we have to add both options to *QMake*:

```bash
# Windows
qmake "CONFIG+=ua_encryption ua_events" -r -tp vc examples.pro
# Linux
qmake "CONFIG+=ua_encryption ua_events" -r examples.pro
```

Events now can be used in the C++ code. To create an event, first is necessary to **subtype** the `QUaBaseEvent` class, for example:

In `myevent.h`:

```c++
#include <QUaBaseEvent>

class MyEvent : public QUaBaseEvent
{
    Q_OBJECT

public:
	Q_INVOKABLE explicit MyEvent(QUaServer *server);

};
```

In `myevent.cpp`:

```c++
#include "myevent.h"

MyEvent::MyEvent(QUaServer *server)
	: QUaBaseEvent(server)
{

}
```

The same rules apply as when subtyping *Objects* or *Variables* (see the *Types* section).

Events must have an **originator** node, which can be any object in the address space that allows to subscribe to events. This is defined in the [`EventNotifier`](https://reference.opcfoundation.org/v104/Core/docs/Part3/8.59/) attribute which can be accesed through the `QUaBaseObject` API:

```c++
quint8 eventNotifier() const;
void setEventNotifier(const quint8 &eventNotifier);
```

The value should be an enumeration, but to simplify the usage, there are a couple of helper methods:

```c++
bool subscribeToEvents() const;
void setSubscribeToEvents(const bool& subscribeToEvents);
```

The `setSubscribeToEvents(true)` enables events for the object while `setSubscribeToEvents(false)` disables them. By default events are **disabled** for all objects. Except for the [**Server Object**](https://reference.opcfoundation.org/v104/Core/docs/Part5/8.3.2/).

If there is an event which does not originate from any object, then is necessary to use the *Server Object* to create and trigger the event. An event is instantiated using the `createEvent<T>()` method:

```c++
auto event = server.createEvent<MyEvent>();
```

Note that for events there is no need to define a `BrowseName` upon instantiation, that is because events are not normally exposed in the address space (with the exception of Alarms and Conditions).

Once an event is created, some [event variables](https://reference.opcfoundation.org/v104/Core/ObjectTypes/BaseEventType/) can be set to define the event information. This is provided by the inherited `QUaBaseEvent` API:

```c++
QString sourceName() const;
void setSourceName(const QString &strSourceName);

QDateTime time() const;
void setTime(const QDateTime &dateTime);

QString message() const;
void setMessage(const QString &strMessage);

quint16 severity() const;
void setSeverity(const quint16 &intSeverity);
```

* `SourceName` : Description of the source of the Event.

* `Time` : Time (in UTC) the Event occurred. It comes from the underlying system or device.

* `Message` : Human-readable description of the Event.

* `Severity` : Urgency of the Event. Value from 1 to 1000, with 1 being the lowest severity and 1000 being the highest.

The variables must be set before triggering the event. Then, the event can be triggered with the `trigger()` method.

In order to be able to test the events though, it is necessary to have a mechanism to trigger events on demand. One option is to create a method to trigger the event:

```c++
auto event = server.createEvent<MyEvent>();

objsFolder->addMethod("triggerServerEvent", [&event]() {
	// set event information
	event->setSourceName("Server");
	event->setMessage("An event occured in the server");
	event->setTime(QDateTime::currentDateTimeUtc());
	event->setSeverity(100);
	// trigger event
	event->trigger();	
});
```

In order to visualize events, some clients require a special events window. For example in *UA Expert*, click the `Add Document` button, then select `Event View` and click `Add`. Then *drag and drop* the *Server Object* (`/Root/Objects/Server`) to the *Configuration* window. Now is possible to see events.

<p align="center">
  <img src="./res/img/08_events_01.jpg">
</p>

The event can be triggered any number of times, and its variables can be updated to new values at any point. Once is not needed anymore, the event can be deleted:

```c++
delete event;
```

If it is desired to trigger events with an specific object as originator, simply create the event using that object's `createEvent<T>()` method:

```c++
QUaFolderObject * objsFolder = server.objectsFolder();
auto obj = objsFolder->addBaseObject("obj");

// Enable object for events
obj->setSubscribeToEvents(true);
// Create event with object as originator
auto obj_event = obj->createEvent<MyEvent>();
```

But now on the client it is necessary to *drag and drop* the originator object to the *Configuration* window.

<p align="center">
  <img src="./res/img/08_events_02.jpg">
</p>

### Events Example

Build and test the events example in [./examples/08_events](./examples/08_events/main.cpp) to learn more.

---

## Serialization

The *QUaServer* supports creating and destroying nodes at *runtime*. Therefore it is possible for a client to modify the server's *Address Space* remotely. This can be achieved, for example, through methods:

```c++
QUaFolderObject * objsFolder = server.objectsFolder();

objsFolder->addMethod("CreateVariable", [objsFolder](QString strVariableName) {
	if (objsFolder->browseChild(strVariableName))
	{
		return QString("Error : Variable %1 already exists.").arg(strVariableName);
	}
	auto newVar = objsFolder->addBaseDataVariable(strVariableName);
	return QString("Success : Variable %1 created.").arg(strVariableName);
});

objsFolder->addMethod("DestroyVariable", [objsFolder](QString strVariableName) {
	auto var = objsFolder->browseChild(strVariableName);
	if (!var)
	{
		return QString("Error : Variable %1 does not exists.").arg(strVariableName);
	}
	delete var;
	return QString("Success : Variable %1 destroyed.").arg(strVariableName);
});
```

Note it is possible to make *UaExpert* refresh the *Address Space* automatically, by compiling the snippet above with `qmake "CONFIG+=ua_events"`.

All the nodes created at *runtime* exist only in memory, so if the server program is restarted, all those nodes will be lost.

To solve this issue, `QUaNode` provides a *serialization* API to help saving the current state of the *Address Space* to disk, and also to be able to restore it from disk.

To save to disk, `QUaNode` provides the `serialize` method:

```c++
template<typename T>
bool serialize(T& serializer, QQueue<QUaLog> &logOut);
```

Where `T` is any C++ *type* implementing the following interface:

```c++
// required API for QUaNode::serialize
bool writeInstance(
	const QUaNodeId& nodeId,
	const QString& typeName,
	const QMap<QString, QVariant>& attrs,
	const QList<QUaForwardReference>& forwardRefs,
	QQueue<QUaLog>& logOut
);
```

When the `serialize` method is called over a node instance, it will call the `serializer`'s `writeInstance` method recursivelly, starting with the node instance that called it and all its descendants. If the `writeInstance` method returns `false`, the recursion stops, and the call to `serialize` also returns `false`. Any useful log messages should be added to the `logOut` queue.

It is then responsability of the `writeInstance` implementation to save to disk the node's information (`nodeId`, `attrs` and `forwardRefs`) in a sensible manner. For example, in the [./examples/09_serialization](./examples/09_serialization) example, the `QUaXmlSerializer` class implements serialization to XML in the following format:

```xml
<?xml version='1.0' encoding='UTF-8'?>
<nodes>
 <n nodeId="ns=0;i=85" browseName="Objects" description="" eventNotifier="0" displayName="Objects" writeMask="0">
  <r forwardName="Organizes" targetType="QUaBaseObject" targetNodeId="ns=1;s=my_obj" inverseName="OrganizedBy"/>
  <r forwardName="Organizes" targetType="QUaFolderObject" targetNodeId="ns=0;i=1592406929" inverseName="OrganizedBy"/>
  <r forwardName="Organizes" targetType="QUaBaseDataVariable" targetNodeId="ns=0;i=2501818547" inverseName="OrganizedBy"/>
  <r forwardName="Organizes" targetType="QUaProperty" targetNodeId="ns=1;s=my_prop" inverseName="OrganizedBy"/>
 </n>
 <n nodeId="ns=1;s=my_obj" browseName="my_object" description="" eventNotifier="0" displayName="my_object" writeMask="0">
  <r forwardName="FriendOf" targetType="TemperatureSensor" targetNodeId="ns=0;i=2070436686" inverseName="FriendOf"/>
  <r forwardName="HasProperty" targetType="QUaProperty" targetNodeId="ns=0;i=2687773104" inverseName="PropertyOf"/>
  <r forwardName="HasOrderedComponent" targetType="QUaBaseObject" targetNodeId="ns=0;i=4261035154" inverseName="OrderedComponentOf"/>
  <r forwardName="HasOrderedComponent" targetType="QUaFolderObject" targetNodeId="ns=0;i=2452012465" inverseName="OrderedComponentOf"/>
 </n>
 <!-- more nodes ... -->
</nodes>
```

It simply writes down a list of nodes, each node with the `<n>` *XML tag* and all the node's attributes as *XML attributes*. Each `<n>` contains a list of `<r>` *XML tags* as children listing the *forward references* for that node. This is all the information required to *serialize* the state of the *Address Space*.

Note that the `typeName` is not serialized as an *XML attribute*. The `typeName` is passed to the `writeInstance` method to allow the user to organize the data by type if desired. In the *XML* serialization this is not necessary, but if serializing to *SQL*, knowing the `typeName` might be useful to store all instance of a type in their own table.

The type `T` can *optionally* implement the following interface:

```c++
// optional API for QUaNode::serialize
bool serializeStart(QQueue<QUaLog>& logOut);

// optional API for QUaNode::serialize
bool serializeEnd(QQueue<QUaLog>& logOut);
```

Implementing such methods can be useful to perform intialization tasks such as opening a file for writing, and to perform clean up tasks such as closing the file or release any other resources.

To serialize all node instances in the *Address Space*, `serialize` should be called over the *Objects Folder* of the server, for example:

```c++
objsFolder->addMethod("Serialize", [objsFolder](QString strFileName) {
	QUaXmlSerializer serializer;
	QQueue<QUaLog> logOut;
	if (!serializer.setXmlFileName(strFileName, logOut))
	{
		return QString("Error in file name.");
	}
	if (!objsFolder->serialize(serializer, logOut))
	{
		return QString("Error serializing. Check the logOut.");
	}
	return QString("Success : Serialized to %1 file.").arg(strFileName);
});
```

To restoring the *Address Space* from disk, `QUaNode` provides the `deserialize` method:

```c++
template<typename T>
bool deserialize(T& deserializer, QQueue<QUaLog>& logOut);
```

Where `T` is any C++ *type* implementing the following interface:

```c++
// required API for QUaNode::deserialize
bool readInstance(
	const QUaNodeId &nodeId,
	const QString &typeName,
	QMap<QString, QVariant> &attrs,
	QList<QUaForwardReference> &forwardRefs,
	QQueue<QUaLog> &logOut
);

// optional API for QUaNode::deserialize
bool deserializeStart(QQueue<QUaLog>& logOut);

// optional API for QUaNode::deserialize
bool deserializeEnd(QQueue<QUaLog>& logOut);
```

Which work in a similar fashion as the serializing interface (`writeInstance`, `serializeStart` and `serializeEnd` respectively).

When deserializing, it is responsability of the `readInstance` implementation to return the node's information (`typeName`, `attrs` and `forwardRefs`) for a given `nodeId` and `typeName`.

The `deserialize` call then takes care of restoring the *Address Space* instatiating any missing nodes or overwriting the attributes of any existing nodes.

To deserialize all node instances in the *Address Space*, `deserialize` should be called over the *Objects Folder* of the server, for example:

```c++
objsFolder->addMethod("Deserialize", [objsFolder](QString strFileName) {
	QUaXmlSerializer serializer;
	QQueue<QUaLog> logOut;
	if (!serializer.setXmlFileName(strFileName, logOut))
	{
		return QString("Error in file name.");
	}
	if (!objsFolder->deserialize(serializer, logOut))
	{
		return QString("Error deserializing. Check the logOut.");
	}
	return QString("Success : Deserialized from %1 file.").arg(strFileName);
});
```

Note that the `readInstance` interface requires the underlying data source to be **queryable** by the `nodeId`. This is not the case for an XML file, therefore the `QUaXmlSerializer` example loads all the contents of the XML into a *queryable* structure in memory. As the number of nodes scale, loading all the contents from disk to memory might not be feasible. Then serializing to a *queryable* database might be a better alternative. In the [./examples/09_serialization](./examples/09_serialization) example, the `QUaSqliteSerializer` class implements serialization to a *queryable* *Sqlite* database.

Both `QUaXmlSerializer` and `QUaSqliteSerializer` classes provided in the [./examples/09_serialization](./examples/09_serialization) example are just to demonstrate the use if the serialization API. They are by no means the best or most efficient way to serialize the *Address Space*, the user should provide their own *serializer* implementation.

### Serialization Example

Build and test the events example in [./examples/09_serialization](./examples/09_serialization/main.cpp) to learn more.

---

## Historizing

The *QUaServer* supports storing *histrical data* and *historical events*, exposing them through the [*HistoryRead* service](https://reference.opcfoundation.org/v104/Core/docs/Part4/5.10.3/).

To enable this functionality, build the library using the Qt project included in this repo using the `ua_historizing` configuration flag:

```bash
cd ./src/amalgamation
# Windows
qmake "CONFIG+=ua_historizing" -tp vc amalgamation.pro
msbuild open62541.vcxproj
# Linux
qmake "CONFIG+=ua_historizing" amalgamation.pro
make all
```

To update the examples to support *historizing*:

```bash
# Windows
qmake "CONFIG+=ua_historizing" -r -tp vc examples.pro
msbuild examples.sln
# Linux
qmake "CONFIG+=ua_historizing" -r examples.pro
make all
```

To support *historizing*, `QUaServer` provides the `setHistorizer` method:

```c++
template<typename T>
bool setHistorizer(T& historizer);
```

### Historizing Data

To historize data, the historizer `T` can be any C++ *type* implementing the following interface:

```c++
// required API for QUaServer::setHistorizer
// write data point to backend, return true on success
bool writeHistoryData(
	const QUaNodeId &nodeId,
	const QUaHistoryDataPoint &dataPoint,
	QQueue<QUaLog>  &logOut
);
// required API for QUaServer::setHistorizer
// update an existing node's data point in backend, return true on success
bool updateHistoryData(
	const QUaNodeId &nodeId, 
	const QUaHistoryDataPoint &dataPoint,
	QQueue<QUaLog>  &logOut
);
// required API for QUaServer::setHistorizer
// remove an existing node's data points within a range, return true on success
bool removeHistoryData(
	cconst QUaNodeId &nodeId, 
	const QDateTime  &timeStart,
	const QDateTime  &timeEnd,
	QQueue<QUaLog>   &logOut
); 
// required API for QUaServer::setHistorizer
// return the timestamp of the first sample available for the given node
QDateTime firstTimestamp(
	const QString  &strNodeId,
	QQueue<QUaLog> &logOut
) const;
// required API for QUaServer::setHistorizer
// return the timestamp of the latest sample available for the given node
QDateTime lastTimestamp(
	const QUaNodeId &nodeId, 
	QQueue<QUaLog>  &logOut
) const;
// required API for QUaServer::setHistorizer
// return true if given timestamp is available for the given node
bool hasTimestamp(
	const QString   &strNodeId,
	const QDateTime &timestamp,
	QQueue<QUaLog>  &logOut
) const;
// required API for QUaServer::setHistorizer
// return a timestamp matching the criteria for the given node
QDateTime findTimestamp(
	const QUaNodeId &nodeId, 
	const QDateTime &timestamp,
	const QUaHistoryBackend::TimeMatch& match,
	QQueue<QUaLog>  &logOut
) const;
// required API for QUaServer::setHistorizer
// return the number for data points within a time range for the given node
quint64 numDataPointsInRange(
	const QUaNodeId &nodeId, 
	const QDateTime &timeStart,
	const QDateTime &timeEnd,
	QQueue<QUaLog>  &logOut
) const;
// required API for QUaServer::setHistorizer
// return the numPointsToRead data points for the given node
// starting from the numPointsOffset offset after given start time (pagination)
QVector<QUaHistoryDataPoint> readHistoryData(
	const QUaNodeId &nodeId, 
	const QDateTime &timeStart,
	const quint64   &numPointsOffset,
	const quint64   &numPointsToRead,
	QQueue<QUaLog>  &logOut
) const;
```

To allow **storing** data it is only necessary to implement `writeHistoryData`, while the other methods can return default values. The data will be saved to whatever media it is desired. 

Implementing only `writeHistoryData`, means clients won't be able to access the historcal data remotely yet, for that it is necessary to implement more methods of the API, as explained further below.

To *store* the data, the API passes the *NodeId* (`const QUaNodeId &nodeId`) of the variable to be historized.

The `QUaHistoryDataPoint` structure (`const QUaHistoryDataPoint &dataPoint`) provides the information that needs to be stored:

```c++
struct QUaHistoryDataPoint
{
	QDateTime timestamp;
	QVariant  value;
	quint32   status;
};
```

Whatever storage media is chosen, it must be *queriable* first, by *NodeId* and second, by *Timestamp*. 

For example, if a `SQL` database is chosen for storage, one approach is to create one table for each *NodeId*. Each table having three columns for *time*, *value* and *status* respectively. To speed up queries, it is recommended to create *indexes* over the *time* column.

Some *pseudo-SQL* code is used in this documentation to illustrate *possible* implementation of each API method. For example, to create each *NodeId* table:

```sql
CREATE TABLE ":NodeId" (
	[:NodeId] INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
	[Time] INTEGER NOT NULL,
	[Value] :DataType NOT NULL,
	[Status] INTEGER NOT NULL
);
-- create index to optimize queries by time
CREATE UNIQUE INDEX ":NodeId_Time" ON ":NodeId"(Time);
```

For `writeHistoryData`:

```sql
INSERT INTO ":NodeId" (Time, Value, Status) VALUES (:Time, :Value, :Status);
```

All the methods of the API should populate the `QQueue<QUaLog> &logOut` parameter with log entries describing any error occurred during the storing or querying process.

To allow an OPC UA Client to **access** the historcal data remotely, it is necessary to further implement the `firstTimestamp`, `lastTimestamp`, `hasTimestamp`, `findTimestamp`, `numDataPointsInRange` and `readHistoryData`. The implementation of this methods is self-describing by their names. Below some *pseudo-SQL* to illustrate *possible* implementation:

For `firstTimestamp`:

```sql
SELECT p.Time FROM ":NodeId" p ORDER BY p.Time ASC LIMIT 1;
```

For `lastTimestamp`:

```sql
SELECT p.Time FROM ":NodeId" p ORDER BY p.Time DESC LIMIT 1;
```

For `hasTimestamp`:

```sql
SELECT COUNT(*) FROM ":NodeId" p WHERE p.Time = :Time;
```

For `findTimestamp`:

```sql
-- from above
SELECT p.Time FROM ":NodeId" p WHERE p.Time > :Time ORDER BY p.Time ASC LIMIT 1;
-- from below
SELECT p.Time FROM ":NodeId" p WHERE p.Time < :Time ORDER BY p.Time DESC LIMIT 1;
```

For `numDataPointsInRange`:

```sql
SELECT COUNT(*) FROM ":NodeId" p WHERE p.Time >= :TimeStart AND p.Time <= :TimeEnd ORDER BY p.Time ASC;
```

For `readHistoryData`:

```sql
SELECT p.Time, p.Value, p.Status FROM":NodeId" p WHERE p.Time >= :Time ORDER BY p.Time ASC LIMIT :Limit OFFSET :Offset;
```

To allow *modifying* historical data, the `updateHistoryData` and `removeHistoryData` should be implemented accordingly.

Finally, to historize a variable, the `QUaBaseVariable::setHistorizing(const bool& historizing)` method should be called. And to allow clients to access its historical data remotelly, the `QUaBaseVariable::setReadHistoryAccess(const bool& readHistoryAccess)` method should be called. For example:

```c++
// create int variable
auto varInt = objsFolder->addBaseDataVariable("MyInt", "ns=0;s=MyInt");
varInt->setValue(0);
// NOTE : must enable historizing for each variable
varInt->setHistorizing(true);
varInt->setReadHistoryAccess(true);
```

Similarly, to allow clients to modify the historical data, the `QUaBaseVariable::setWriteHistoryAccess(const bool& bHistoryWrite)` method should be called.

<p align="center">
  <img src="./res/img/10_historizing_01_data.gif">
</p>

### Historizing Events

Historizing events is only possible if the `QUaServer` project is compiled using the `CONFIG+=ua_events` flag. See the *Events* section of this document for more information.

To historize events, the historizer `T` can be any C++ *type* implementing the following interface:

```c++
// write a event's data to backend
bool writeHistoryEventsOfType(
	const QUaNodeId            &eventTypeNodeId,
	const QList<QUaNodeId>     &emittersNodeIds,
	const QUaHistoryEventPoint &eventPoint,
	QQueue<QUaLog>             &logOut
);
// get event types (node ids) for which there are events stored for the given emitter
QVector<QUaNodeId> eventTypesOfEmitter(
	const QUaNodeId &emitterNodeId,
	QQueue<QUaLog>  &logOut
);
// find a timestamp matching the criteria for the emitter and event type
QDateTime findTimestampEventOfType(
	const QUaNodeId                    &emitterNodeId,
	const QUaNodeId                    &eventTypeNodeId,
	const QDateTime                    &timestamp,
	const QUaHistoryBackend::TimeMatch &match,
	QQueue<QUaLog>                     &logOut
);
// get the number for events within a time range for the given emitter and event type
quint64 numEventsOfTypeInRange(
	const QUaNodeId &emitterNodeId,
	const QUaNodeId &eventTypeNodeId,
	const QDateTime &timeStart,
	const QDateTime &timeEnd,
	QQueue<QUaLog>  &logOut
);
// return the numPointsToRead events for the given emitter and event type,
// starting from the numPointsOffset offset after given start time (pagination)
QVector<QUaHistoryEventPoint> readHistoryEventsOfType(
	const QUaNodeId &emitterNodeId,
	const QUaNodeId &eventTypeNodeId,
	const QDateTime &timeStart,
	const quint64   &numPointsOffset,
	const quint64   &numPointsToRead,
	const QList<QUaBrowsePath> &columnsToRead,
	QQueue<QUaLog>  &logOut
);
```

To allow **storing** events it is only necessary to implement `writeHistoryEventsOfType`, while the other methods can return default values. The events will be saved to whatever media it is desired. 

Implementing only `writeHistoryEventsOfType`, means clients won't be able to access the historcal events remotely yet, for that it is necessary to implement more methods of the API, as explained further below.

To *store* the events, the API passes the *NodeId* (`const QUaNodeId &eventTypeNodeId`) of the **event type** to be historized, a list of *emitter* nodeIds and the event data.

The `QUaHistoryEventPoint` structure provides the information that needs to be stored:

```c++
struct QUaHistoryEventPoint
{
	QDateTime timestamp;
	QHash<QUaBrowsePath, QVariant> fields;
};
```

Historizing events is slightly more complicated than historizing data. Mainly because *historical data* has always the same struture (`{timestamp, value, status}`), while *event data* changes depending on the *event type*, and there can be any number of event types, including the custom ones. 

Another complication is that in OPC UA, the same event can be *emitted* or *notified* by different objects (objects that are not necessarily be the event's `SourceNode`) according to a [*Event Refereneces*](https://reference.opcfoundation.org/v104/Core/docs/Part3/7.18/) organization defined by the OPC specification. So when the event history is queried for a *notifier* node, all the events that were emitted by this node must be retrieved. This relation is specified by the `const QList<QUaNodeId> &emittersNodeIds` argument in the `writeHistoryEventsOfType` historic API method.

Whatever storage media is chosen, events must be *queriable* first by *EventType*, then by *Emitter*, and finally by *Timestamp*. 

For example, if a `SQL` database is chosen for storage, one approach is to create one table for each *EventType*. Each table having a fixed number of columns according to the fixed amout of event fields an *EventType* has. The `const QUaHistoryEventPoint &eventPoint` argument if the `writeHistoryEventsOfType` API method is always guaranteed to contain the same event fields for a given type. So the `QUaHistoryEventPoint::fields` information can be used to create the *EventType* tables.

```sql
CREATE TABLE ":EventTypeNodeId" ( :EventFieldNames :EventFieldTypes );
```

Then create one able to store the *EventType* table names, to be able to relate them to a unique index.

```sql
CREATE TABLE "EventTypeTableNames"
(
	[EventTypeTableNames] INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
	[TableName] TEXT NOT NULL
);
```

To speed up queries, it is recommended to create an *index* over the *TableName* column.

Then a table per *Emitter* can be created with fixed a number of columns that help query and relate to the events stored in the *EventType* tables.

```sql
CREATE TABLE ":EmitterNodeId"
(
	[:EmitterNodeId] INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
	[Time] INTEGER NOT NULL,      -- to be able to query emitter's events by time range
	[EventType] INTEGER NOT NULL, -- index of EventTypeTableNames table
	[EventId] INTEGER NOT NULL    -- index of event in its EventType table
);
```

To speed up queries, it is recommended to create an *index* over the *Time* and *EventType* columns.

```sql
CREATE INDEX ":EmitterNodeId_Time_EventType" ON ":EmitterNodeId" (Time, EventType);
```

Then the procedure to store an event when the `writeHistoryEventsOfType` API method is called would be:

* Insert new event in its *EventType* table, return new event's *EventType* key.

* Check if the (*EventType*) *TableName* already in the *EventTypeTableNames* table else insert it, fetch the *EventTypeTableNames* key.

* Insert the key of the new event and event type in each emitter table.

Then the rest of the API to query the event history could be imlpemented as follows:

For `eventTypesOfEmitter`:

```sql
SELECT n.TableName FROM EventTypeTableNames n 
INNER JOIN 
(
	SELECT DISTINCT EventType FROM ":EmitterNodeId"
) e
ON n.EventTypeTableNames = e.EventType
```

For `findTimestampEventOfType`:

```sql
-- from above
SELECT e.Time FROM ":EmitterNodeId" e WHERE e.Time >= :Time AND e.EventType = :EventTypeKey ORDER BY e.Time ASC LIMIT 1;
-- from below
SELECT e.Time FROM ":EmitterNodeId" e WHERE e.Time < :Time AND e.EventType = :EventTypeKey ORDER BY e.Time DESC LIMIT 1;
```

For `numEventsOfTypeInRange`:

```sql
SELECT COUNT(*) FROM ":EmitterNodeId" e WHERE e.Time >= :TimeStart AND e.Time <= :TimeEnd AND e.EventType = :EventTypeKey ORDER BY e.Time ASC;
```

For `readHistoryEventsOfType` :

```sql
SELECT * FROM ":EventTypeNodeId" t 
INNER JOIN 
(
	SELECT EventId FROM ":EmitterNodeId" e WHERE e.Time >= :TimeStart AND e.EventType = :EventTypeKey ORDER BY e.Time ASC LIMIT :Limit OFFSET :Offset
) e
ON t.EventTypeNodeId = e.EventId;
```

<p align="center">
  <img src="./res/img/10_historizing_02_events.gif">
</p>

### Historizing Example

The [`quainmemoryhistorizer.cpp`](./examples/10_historizing/quainmemoryhistorizer.cpp) file shows an example of historical data and event storage in memory, while the [`quasqlitehistorizer.cpp`](./examples/10_historizing/quasqlitehistorizer.cpp) file shows an example of historical storage using *Sqlite*.

Note that these examples are provided for illustration purposes only and not for production. The user is encouraged to implement (and if possible, share) their own historizer implementations.

Build and test the historizing example in [./examples/10_historizing](./examples/10_historizing/main.cpp) to learn more.

---

## Alarms

At the time of writing, alarms and conditions are considered an `EXPERIMENTAL` feature in the *open62541* library, therefore the same applies for *QUaServer*. Please use with caution.

To use alarms and conditions, it is necessary to create a new amalgamation from the *open62541* source code that supports alarms. This can be done by building it with the following commands:

```bash
cd ./depends/open62541.git
mkdir build; cd build
# Adjust your Cmake generator accordingly
cmake -DUA_ENABLE_AMALGAMATION=ON -DUA_NAMESPACE_ZERO=FULL -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON -DUA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS=ON .. -G "Visual Studio 15 2017 Win64"
```

* The `-DUA_NAMESPACE_ZERO=FULL` option is needed because by default *open62541* does not include the complete address space of the OPC UA standard in order to reduce binary size. But to support events, it is actually necessary to have the `FULL` address space available in the server application.

* The `-DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON` is the flag that enables events.

* The `-DUA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS=ON` is the flag that enables alarms and conditions.

Note that the amalgamation files are now considerably larger because now they contain the full default OPC UA address space.

Now build the library using the Qt project included in this repo:

```bash
cd ./src/amalgamation
# Windows
qmake "CONFIG+=ua_alarms_conditions" -tp vc amalgamation.pro
msbuild open62541.vcxproj
# Linux
qmake "CONFIG+=ua_alarms_conditions" amalgamation.pro
make all
```

To update the examples to support events:

```bash
# Windows
qmake "CONFIG+=ua_alarms_conditions" -r -tp vc examples.pro
msbuild examples.sln
# Linux
qmake "CONFIG+=ua_alarms_conditions" -r examples.pro
make all
```

After running `qmake` it is often necessary to **rebuild** the application to avoid *missing symbols* errors.

Two types of alarms are available out of the box by the `QUaServer` API:

* `QUaOffNormalAlarm` : Used for alarms based on discrete values. Useful not only for alarms based on *boolean* values, but also any other discrete values such as *integers*.

* `QUaExclusiveLevelAlarm` : Used for alarms based in continuous numeric values. It provides automatic level checking.

### QUaOffNormalAlarm

To create a `QUaOffNormalAlarm`, the first step is to create an object that will be the `SourceNode` of the events triggered by the alarm. Clients will be then able to subscribe to events emitted by this object in order to track the alarm state.

```c++
auto motionSensor = objsFolder->addChild<QUaBaseObject>("motionSensor");
```

Then a variable is needed that will provide the discrete value that the alarm will monitor. Any variable that contains a discrete value can be used.

```c++
auto moving = motionSensor->addBaseDataVariable("moving");
moving->setWriteAccess(true);
moving->setDataType(QMetaType::Bool);
moving->setValue(false);
```

Finally the `QUaOffNormalAlarm` can be created based on its `SourceNode`, setting the variable with the discrete value as an `InputNode` and defining what the *Normal Value* of the `InputNode` should be.

```c++
auto motionAlarm = motionSensor->addChild<QUaOffNormalAlarm>("alarm");
motionAlarm->setConditionName("Motion Sensor Alarm");
motionAlarm->setInputNode(moving);
motionAlarm->setNormalValue(false);
motionAlarm->setConfirmRequired(true);
```

For the alarm to start generating events, first it has to be **enabled**. This can be done by calling the `Enable` method of the alarm object through the network using an OPC client or programmatically using the C++ `Enable()` method.

<p align="center">
  <img src="./res/img/11_alarms_01_offnormal.gif">
</p>

### QUaExclusiveLevelAlarm

To create a `QUaExclusiveLevelAlarm`, the first step is to create an object that will be the `SourceNode` of the events triggered by the alarm. Clients will be then able to subscribe to events emitted by this object in order to track the alarm state.

```c++
auto levelSensor = objsFolder->addChild<QUaBaseObject>("levelSensor");
```

Then a variable is needed that will provide the continuous value that the alarm will monitor. Any variable that contains a continuous value can be used.

```c++
auto level = levelSensor->addBaseDataVariable("level");
level->setWriteAccess(true);
level->setDataType(QMetaType::Double);
level->setValue(0.0);
```

Then the `QUaExclusiveLevelAlarm` can be created based on its `SourceNode`, setting the variable with the continuous value as an `InputNode`. 

```c++
auto levelAlarm = levelSensor->addChild<QUaExclusiveLevelAlarm>("alarm");
levelAlarm->setConditionName("Level Sensor Alarm");
levelAlarm->setInputNode(level);

levelAlarm->setHighLimitRequired(true);
levelAlarm->setLowLimitRequired(true);
levelAlarm->setHighLimit(10.0);
levelAlarm->setLowLimit(-10.0);
```

By default the `QUaExclusiveLevelAlarm` does not monitor any limits, so they have to be required explicitly using the `QUaExclusiveLevelAlarm` API:

```c++
// to enabled the monitoring of specific limits
void setHighHighLimitRequired(const bool& highHighLimitRequired);
void setHighLimitRequired    (const bool& highLimitRequired    );
void setLowLimitRequired     (const bool& lowLimitRequired     );
void setLowLowLimitRequired  (const bool& lowLowLimitRequired  );

// to define the limits
double highHighLimit() const;
void setHighHighLimit(const double& highHighLimit);

double highLimit() const;
void setHighLimit(const double& highLimit);

double lowLimit() const;
void setLowLimit(const double& lowLimit);

double lowLowLimit() const;
void setLowLowLimit(const double& lowLowLimit);
```

For the alarm to start generating events, first it has to be **enabled**. This can be done by calling the `Enable` method of the alarm object through the network using an OPC client or programmatically using the C++ `Enable()` method.

<p align="center">
  <img src="./res/img/11_alarms_02_level.gif">
</p>

### Branches

Support for [branches](https://reference.opcfoundation.org/v104/Core/docs/Part9/5.5.3/) in `QUaServer` is disabled by default. To enable branches call the `setBranchQueueSize` method with a value larger than `0`. This will create a branch queue in the alarm which will keep the given number of branches in memory. If more branches are created than the size of the queue, the oldest branch will be deleted automatically to avoid memory saturation.

```c++
motionAlarm->setBranchQueueSize(10);
levelAlarm->setBranchQueueSize(10);
```

Historizing of branches is also disabled by default, to enable it, call the `setHistorizingBranches` method with a `true` value.

```c++
motionAlarm->setHistorizingBranches(true);
levelAlarm->setHistorizingBranches(true);
```

---

## License

### Amalgamation

The amalgamation source code found in `./src/amalgamation` is licensed by **open62541** under the [Mozilla Public License 2.0](https://github.com/open62541/open62541/blob/master/LICENSE).

### QUaTypesConverter

The source code in the files `./src/wrapper/quatypesconverter.h` and `quatypesconverter.cpp` was copied and adapted from the [QtOpcUa repository](https://github.com/qt/qtopcua) (files [qopen62541valueconverter.h](https://github.com/qt/qtopcua/blob/5.12/src/plugins/opcua/open62541/qopen62541valueconverter.h) and [qopen62541valueconverter.cpp](https://github.com/qt/qtopcua/blob/5.12/src/plugins/opcua/open62541/qopen62541valueconverter.cpp)) and is under the [LGPL license](https://github.com/qt/qtopcua/blob/5.12/LICENSE.LGPLv3).

### QUaServer

For the rest of the code, the license is [MIT](https://opensource.org/licenses/MIT).

Copyright (c) 2019 -2020 Juan Gonzalez Burgos
