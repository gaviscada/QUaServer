#ifndef QUACUSTOMDATATYPES_H
#define QUACUSTOMDATATYPES_H

#include <QObject>
#include <QVariant>
#include <QUuid>
#include <QRegularExpression>
#include <QDate>
#include <QTimeZone>
#include <QQueue>
#include <QDebug>

#include <open62541.h>

class QUaNode;

Q_DECLARE_METATYPE(QTimeZone);

#define METATYPE_OFFSET_LOCALIZEDTEXT 1
#define METATYPE_LOCALIZEDTEXT (QMetaType::Type)(QMetaType::User + METATYPE_OFFSET_LOCALIZEDTEXT)

#define METATYPE_OFFSET_TIMEZONEDATATYPE 2
#define METATYPE_TIMEZONEDATATYPE (QMetaType::Type)(QMetaType::User + METATYPE_OFFSET_TIMEZONEDATATYPE)

#define METATYPE_OFFSET_NODEID 3
#define METATYPE_NODEID (QMetaType::Type)(QMetaType::User + METATYPE_OFFSET_NODEID)

#define METATYPE_OFFSET_CHANGESTRUCTUREDATATYPE 4
#define METATYPE_CHANGESTRUCTUREDATATYPE (QMetaType::Type)(QMetaType::User + METATYPE_OFFSET_CHANGESTRUCTUREDATATYPE)

#define METATYPE_OFFSET_IMAGE 5
#define METATYPE_IMAGE (QMetaType::Type)(QMetaType::User + METATYPE_OFFSET_IMAGE)

#define METATYPE_OFFSET_STATUSCODE 6
#define METATYPE_STATUSCODE (QMetaType::Type)(QMetaType::User + METATYPE_OFFSET_STATUSCODE)

// traits used to static assert that a method cannot be used
// https://stackoverflow.com/questions/24609872/delete-virtual-function-from-a-derived-class
template <typename T>
struct QUaFail : std::false_type
{
};

// to have UA_NodeId as a hash key
inline bool operator==(const UA_NodeId &e1, const UA_NodeId &e2)
{
	return e1.namespaceIndex     == e2.namespaceIndex
		&& e1.identifierType     == e2.identifierType
		&& (e1.identifierType == UA_NODEIDTYPE_NUMERIC    ? e1.identifier.numeric == e2.identifier.numeric :
			e1.identifierType == UA_NODEIDTYPE_STRING     ? UA_String_equal    (&e1.identifier.string    , &e2.identifier.string    ) :
			e1.identifierType == UA_NODEIDTYPE_GUID       ? UA_Guid_equal      (&e1.identifier.guid      , &e2.identifier.guid      ) :
			e1.identifierType == UA_NODEIDTYPE_BYTESTRING ? UA_ByteString_equal(&e1.identifier.byteString, &e2.identifier.byteString) : false);
}

inline uint qHash(const UA_NodeId &key, uint seed)
{
	return qHash(key.namespaceIndex, seed) ^ qHash(key.identifierType, seed) ^ (key.identifierType == UA_NODEIDTYPE_NUMERIC ? qHash(key.identifier.numeric, seed) : UA_NodeId_hash(&key));
}

struct QUaReferenceType
{
	QString strForwardName;
	QString strInverseName;
};
Q_DECLARE_METATYPE(QUaReferenceType);

QDebug operator<<(QDebug debug, const QUaReferenceType& refType);

// to have QUaReferenceType as a hash key
inline bool operator==(const QUaReferenceType& e1, const QUaReferenceType& e2)
{
	return e1.strForwardName.compare(e2.strForwardName, Qt::CaseSensitive) == 0
		&& e1.strInverseName.compare(e2.strInverseName, Qt::CaseSensitive) == 0;
}

inline bool operator!=(const QUaReferenceType& e1, const QUaReferenceType& e2)
{
	return !(e1 == e2);
}

inline uint qHash(const QUaReferenceType& key, uint seed)
{
	return qHash(key.strForwardName, seed) ^ qHash(key.strInverseName, seed);
}

struct QUaForwardReference
{
	QString targetNodeId;
	QString targetType;
	QUaReferenceType refType;
};

inline bool operator==(const QUaForwardReference& e1, const QUaForwardReference& e2)
{
	return e1.targetNodeId.compare(e2.targetNodeId, Qt::CaseSensitive) == 0
		&& e1.refType == e2.refType;
}

namespace QUa
{
	Q_NAMESPACE

	enum class Type
	{
		Bool        = QMetaType::Bool,
		Char        = QMetaType::Char,
		SChar       = QMetaType::SChar,
		UChar       = QMetaType::UChar,
		Short       = QMetaType::Short,
		UShort      = QMetaType::UShort,
		Int         = QMetaType::Int,
		UInt        = QMetaType::UInt,
		Long        = QMetaType::Long,
		LongLong    = QMetaType::LongLong,
		ULong       = QMetaType::ULong,
		ULongLong   = QMetaType::ULongLong,
		Float       = QMetaType::Float,
		Double      = QMetaType::Double,
		QString     = QMetaType::QString,
		QDateTime   = QMetaType::QDateTime,
		QUuid       = QMetaType::QUuid,
		QByteArray  = QMetaType::QByteArray,
		UnknownType = QMetaType::UnknownType,
		LocalizedText           = METATYPE_LOCALIZEDTEXT,
		TimeZoneDataType        = METATYPE_TIMEZONEDATATYPE,
		NodeId                  = METATYPE_NODEID,
		ChangeStructureDatatype = METATYPE_CHANGESTRUCTUREDATATYPE,
		Image                   = METATYPE_IMAGE,
		StatusCode              = METATYPE_STATUSCODE
	};
	Q_ENUM_NS(Type)

	// Part 8 - 6.3.2 Operation level result codes
	enum class Status
	{
		Good                                    = static_cast<int>(UA_STATUSCODE_GOOD),              
		GoodLocalOverride                       = static_cast<int>(UA_STATUSCODE_GOODLOCALOVERRIDE), 
		Uncertain                               = static_cast<int>(0x40000000),                      
		UncertainNoCommunicationLastUsableValue = static_cast<int>(UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE),
		UncertainLastUsableValue                = static_cast<int>(UA_STATUSCODE_UNCERTAINLASTUSABLEVALUE),
		UncertainSubstituteValue                = static_cast<int>(UA_STATUSCODE_UNCERTAINSUBSTITUTEVALUE),
		UncertainInitialValue                   = static_cast<int>(UA_STATUSCODE_UNCERTAININITIALVALUE),
		UncertainSensorNotAccurate              = static_cast<int>(UA_STATUSCODE_UNCERTAINSENSORNOTACCURATE),
		UncertainEngineeringUnitsExceeded       = static_cast<int>(UA_STATUSCODE_UNCERTAINENGINEERINGUNITSEXCEEDED),
		UncertainSubNormal                      = static_cast<int>(UA_STATUSCODE_UNCERTAINSUBNORMAL),
		Bad                                     = static_cast<int>(0x80000000),                      
		BadConfigurationError	                = static_cast<int>(UA_STATUSCODE_BADCONFIGURATIONERROR),
		BadNotConnected			                = static_cast<int>(UA_STATUSCODE_BADNOTCONNECTED),
		BadDeviceFailure		                = static_cast<int>(UA_STATUSCODE_BADDEVICEFAILURE),
		BadSensorFailure		                = static_cast<int>(UA_STATUSCODE_BADSENSORFAILURE),
		BadOutOfService			                = static_cast<int>(UA_STATUSCODE_BADOUTOFSERVICE),
		BadDeadbandFilterInvalid                = static_cast<int>(UA_STATUSCODE_BADDEADBANDFILTERINVALID)
	};
	Q_ENUM_NS(Status)

	enum class LogLevel {
		Trace   = UA_LogLevel::UA_LOGLEVEL_TRACE,
		Debug   = UA_LogLevel::UA_LOGLEVEL_DEBUG,
		Info    = UA_LogLevel::UA_LOGLEVEL_INFO,
		Warning = UA_LogLevel::UA_LOGLEVEL_WARNING,
		Error   = UA_LogLevel::UA_LOGLEVEL_ERROR,
		Fatal   = UA_LogLevel::UA_LOGLEVEL_FATAL
	};
	Q_ENUM_NS(LogLevel)

	enum class LogCategory {
		Network        = UA_LogCategory::UA_LOGCATEGORY_NETWORK,
		SecureChannel  = UA_LogCategory::UA_LOGCATEGORY_SECURECHANNEL,
		Session        = UA_LogCategory::UA_LOGCATEGORY_SESSION,
		Server         = UA_LogCategory::UA_LOGCATEGORY_SERVER,
		Client         = UA_LogCategory::UA_LOGCATEGORY_CLIENT,
		UserLand       = UA_LogCategory::UA_LOGCATEGORY_USERLAND,
		SecurityPolicy = UA_LogCategory::UA_LOGCATEGORY_SECURITYPOLICY,
		Serialization,
		History,
		Application
	};
	Q_ENUM_NS(LogCategory)
}
typedef QUa::LogLevel    QUaLogLevel;
typedef QUa::LogCategory QUaLogCategory;
typedef QUa::Status      QUaStatus;

struct QUaLog
{
	// default constructor required by Qt
	QUaLog();
	// consutructor accepting QString instead of QByteArray (to support generating messages with QObject::tr)
	QUaLog(const QString& strMessage, const QUaLogLevel& logLevel, const QUaLogCategory& logCategory);
	// members
	QByteArray     message;
	QUaLogLevel    level;
	QUaLogCategory category;
	QDateTime      timestamp;
};
Q_DECLARE_METATYPE(QUaLog);


union QUaWriteMask
{
	struct bit_map {
		bool bAccessLevel             : 1; // UA_WRITEMASK_ACCESSLEVEL
		bool bArrrayDimensions        : 1; // UA_WRITEMASK_ARRRAYDIMENSIONS
		bool bBrowseName              : 1; // UA_WRITEMASK_BROWSENAME
		bool bContainsNoLoops         : 1; // UA_WRITEMASK_CONTAINSNOLOOPS
		bool bDataType                : 1; // UA_WRITEMASK_DATATYPE
		bool bDescription             : 1; // UA_WRITEMASK_DESCRIPTION
		bool bDisplayName             : 1; // UA_WRITEMASK_DISPLAYNAME
		bool bEventNotifier           : 1; // UA_WRITEMASK_EVENTNOTIFIER
		bool bExecutable              : 1; // UA_WRITEMASK_EXECUTABLE
		bool bHistorizing             : 1; // UA_WRITEMASK_HISTORIZING
		bool bInverseName             : 1; // UA_WRITEMASK_INVERSENAME
		bool bIsAbstract              : 1; // UA_WRITEMASK_ISABSTRACT
		bool bMinimumSamplingInterval : 1; // UA_WRITEMASK_MINIMUMSAMPLINGINTERVAL
		bool bNodeClass               : 1; // UA_WRITEMASK_NODECLASS
		bool bNodeId                  : 1; // UA_WRITEMASK_NODEID
		bool bSymmetric               : 1; // UA_WRITEMASK_SYMMETRIC
		bool bUserAccessLevel         : 1; // UA_WRITEMASK_USERACCESSLEVEL
		bool bUserExecutable          : 1; // UA_WRITEMASK_USEREXECUTABLE
		bool bUserWriteMask           : 1; // UA_WRITEMASK_USERWRITEMASK
		bool bValueRank               : 1; // UA_WRITEMASK_VALUERANK
		bool bWriteMask               : 1; // UA_WRITEMASK_WRITEMASK
		bool bValueForVariableType    : 1; // UA_WRITEMASK_VALUEFORVARIABLETYPE
	} bits;
	quint32 intValue;
	// constructors
	QUaWriteMask()
	{
		// all attributes writable by default (getUserRightsMask_default returns 0xFFFFFFFF)
		bits.bAccessLevel             = true;
		bits.bArrrayDimensions        = true;
		bits.bBrowseName              = true;
		bits.bContainsNoLoops         = true;
		bits.bDataType                = true;
		bits.bDescription             = true;
		bits.bDisplayName             = true;
		bits.bEventNotifier           = true;
		bits.bExecutable              = true;
		bits.bHistorizing             = true;
		bits.bInverseName             = true;
		bits.bIsAbstract              = true;
		bits.bMinimumSamplingInterval = true;
		bits.bNodeClass               = true;
		bits.bNodeId                  = true;
		bits.bSymmetric               = true;
		bits.bUserAccessLevel         = true;
		bits.bUserExecutable          = true;
		bits.bUserWriteMask           = true;
		bits.bValueRank               = true;
		bits.bWriteMask               = true;
		bits.bValueForVariableType    = true;
	};
	QUaWriteMask(const quint32& value)
	{
		intValue = value;
	};
};

union QUaAccessLevel
{
	struct bit_map {
		bool bRead           : 1; // UA_ACCESSLEVELMASK_READ
		bool bWrite          : 1; // UA_ACCESSLEVELMASK_WRITE
		bool bHistoryRead    : 1; // UA_ACCESSLEVELMASK_HISTORYREAD
		bool bHistoryWrite   : 1; // UA_ACCESSLEVELMASK_HISTORYWRITE
		bool bSemanticChange : 1; // UA_ACCESSLEVELMASK_SEMANTICCHANGE
		bool bStatusWrite    : 1; // UA_ACCESSLEVELMASK_STATUSWRITE
		bool bTimestampWrite : 1; // UA_ACCESSLEVELMASK_TIMESTAMPWRITE
	} bits;
	quint8 intValue;
	// constructors
	QUaAccessLevel()
	{
		// read only by default
		bits.bRead           = true;
		bits.bWrite          = false;
		bits.bHistoryRead    = false;
		bits.bHistoryWrite   = false;
		bits.bSemanticChange = false;
		bits.bStatusWrite    = false;
		bits.bTimestampWrite = false;
	};
	QUaAccessLevel(const quint8& value)
	{
		intValue = value;
	};
};

class QUaDataType
{

public:
    QUaDataType();
	QUaDataType(const QMetaType::Type& metaType);
    QUaDataType(const QString& strType);
    QUaDataType(const QByteArray& byteType);
	operator QMetaType::Type() const;
	operator QString() const;
	bool operator==(const QMetaType::Type& metaType);
	void operator=(const QString& strType);

private:
	QUa::Type m_type;
	static QMetaEnum m_metaEnum;
};

Q_DECLARE_METATYPE(QUaDataType);

inline uint qHash(const QUaStatus& key, uint seed)
{
	Q_UNUSED(seed);
	return static_cast<uint>(key);
}

class QUaStatusCode
{

public:
	QUaStatusCode();
	QUaStatusCode(const QUaStatus& uaStatus);
	QUaStatusCode(const UA_StatusCode& intStatus);
	QUaStatusCode(const QString& strStatus);
	QUaStatusCode(const QByteArray& byteStatus);
	operator QUaStatus() const;
	operator UA_StatusCode() const;
	operator QString() const;
	bool operator==(const QUaStatus& uaStatus);
	void operator=(const QString& strStatus);

	static QString longDescription(const QUaStatusCode& statusCode);

private:
	QUaStatus m_status;
	static QMetaEnum m_metaEnum;
	static QHash<QUaStatus, QString> m_descriptions;
};

Q_DECLARE_METATYPE(QUaStatusCode);

class QUaQualifiedName
{

public:
	QUaQualifiedName();
	QUaQualifiedName(const quint16 &namespaceIndex, const QString &name);
	QUaQualifiedName(const UA_QualifiedName& uaQualName);
	QUaQualifiedName(const QString& strXmlQualName);
	QUaQualifiedName(const char * strXmlQualName);
	operator UA_QualifiedName() const; // needs cleanup with UA_QualifiedName_clear after use
	operator QString() const;
	void operator=(const UA_QualifiedName& uaQualName);
	void operator=(const QString& strXmlQualName);
	void operator=(const char * strXmlQualName);
	bool operator==(const QUaQualifiedName &other) const;
	bool operator!=(const QUaQualifiedName &other) const;

	quint16 namespaceIndex() const;
	void setNamespaceIndex(const quint16& namespaceIndex);

	QString name() const;
	void seName(const QString& name);

	QString toXmlString() const;
	UA_QualifiedName toUaQualifiedName(); // needs cleanup with UA_QualifiedName_clear after use

	// helpers

	static QUaQualifiedName fromXmlString(const QString& strXmlQualName);
	static QUaQualifiedName fromUaQualifiedName(const UA_QualifiedName& uaQualName);

	typedef QList<QUaQualifiedName> QUaQualifiedNameList;
	inline static QString reduce(const QUaQualifiedNameList& browsePath)
	{
		QString strRet;
		std::for_each(browsePath.begin(), browsePath.end(),
		[&strRet](const QUaQualifiedName& browseName) {
			strRet += browseName.toXmlString() + "/";
		});
		return strRet;
	}

private:
	quint16 m_namespace;
	QString m_name;
};

Q_DECLARE_METATYPE(QUaQualifiedName);

typedef QList<QUaQualifiedName> QUaQualifiedNameList;

inline uint qHash(const QUaQualifiedName& key, uint seed)
{
	return qHash(key.name(), seed) ^ key.namespaceIndex();
}

struct QUaChangeStructureDataType
{
	enum Verb
	{
		NodeAdded        = 1,
		NodeDeleted      = 2,
		ReferenceAdded   = 4,
		ReferenceDeleted = 8,
		DataTypeChanged  = 16
	};
	QUaChangeStructureDataType();
	QUaChangeStructureDataType(
		const QString &strNodeIdAffected, 
		const QString &strNodeIdAffectedType, 
		const Verb    &uiVerb
	);

	QString m_strNodeIdAffected;
	QString m_strNodeIdAffectedType;
	uchar   m_uiVerb;
};
typedef QUaChangeStructureDataType::Verb QUaChangeVerb;
typedef QVector<QUaChangeStructureDataType> QUaChangesList;

inline bool operator==(const QUaChangeStructureDataType& lhs, const QUaChangeStructureDataType& rhs) 
{
	return lhs.m_strNodeIdAffected    .compare(rhs.m_strNodeIdAffected    , Qt::CaseInsensitive) == 0 &&
		   lhs.m_strNodeIdAffectedType.compare(rhs.m_strNodeIdAffectedType, Qt::CaseInsensitive) == 0 &&
		   lhs.m_uiVerb == rhs.m_uiVerb;
}

Q_DECLARE_METATYPE(QUaChangeStructureDataType);


// Enum Stuff
typedef qint64 QUaEnumKey;
struct QUaEnumEntry
{
	QByteArray strDisplayName;
	QByteArray strDescription;
};
Q_DECLARE_METATYPE(QUaEnumEntry);
inline bool operator==(const QUaEnumEntry& lhs, const QUaEnumEntry& rhs) 
{ 
	return lhs.strDisplayName == rhs.strDisplayName && lhs.strDescription == rhs.strDescription;
}
typedef QMap<QUaEnumKey, QUaEnumEntry> QUaEnumMap;
typedef QMapIterator<QUaEnumKey, QUaEnumEntry> QUaEnumMapIter;

// User validation
typedef std::function<bool(const QString &, const QString &)> QUaValidationCallback;

// Class whose only pupose is emit signals
class QUaSignaler : public QObject
{
	Q_OBJECT
public:
    explicit QUaSignaler(QObject *parent = nullptr) 
        : QObject(parent) 
    { 
        m_processing = false;
        QObject::connect(
            this,
            &QUaSignaler::sendEvent,
            this,
            &QUaSignaler::on_sendEvent,
            Qt::QueuedConnection
        );
    };
    template <typename M1 = const std::function<void(void)>&>
    inline void execLater(M1&& func)
    {
        m_funcs.enqueue(func);
        if (m_processing)
        {
            return;
        }
        m_processing = true;
        emit this->sendEvent(QPrivateSignal());
    };
    inline bool processing() const
    {
        return m_processing;
    };
signals:
	void signalNewInstance(QUaNode *node);
    // can only be emitted internally
    void sendEvent(QPrivateSignal);
private slots:
    inline void on_sendEvent()
    {
        Q_ASSERT(m_processing);
        if (m_funcs.isEmpty())
        {
            m_processing = false;
            return;
        }
        m_funcs.dequeue()();
        emit this->sendEvent(QPrivateSignal());
    };
private:
    bool m_processing;
    QQueue<std::function<void(void)>> m_funcs;
};

class QUaSession : public QObject
{
	friend class QUaServer;
	Q_OBJECT

	Q_PROPERTY(QString   sessionId       READ sessionId      )
	Q_PROPERTY(QString   userName        READ userName       )
	Q_PROPERTY(QString   applicationName READ applicationName)
	Q_PROPERTY(QString   applicationUri  READ applicationUri )
	Q_PROPERTY(QString   productUri      READ productUri     )
	Q_PROPERTY(QString   address         READ address        )
	Q_PROPERTY(quint16   port            READ port           )
    Q_PROPERTY(QDateTime timestamp       READ timestamp      )

public:

    explicit QUaSession(QObject* parent = nullptr);

	QString   sessionId     () const;
	QString   userName       () const;
	QString   applicationName() const;
	QString   applicationUri () const;
	QString   productUri     () const;
	QString   address        () const;
	quint16   port           () const;
    QDateTime timestamp      () const;

private:
	QString   m_strSessionId;
	QString   m_strUserName;
	QString   m_strApplicationName;
	QString   m_strApplicationUri;
	QString   m_strProductUri;
	QString   m_strAddress;
	quint16   m_intPort;
    QDateTime m_timestamp;
};

#endif // QUACUSTOMDATATYPES_H
