struct ${classname}::Data: private QScxmlTableData {
    Data(${classname} &stateMachine)
        : stateMachine(stateMachine)
    {}

    void init() {
        stateMachine.setTableData(this);
        ${dataModelInitialization}
    }

    QString name() const override final
    { return ${name}; }

    QScxmlExecutableContent::ContainerId initialSetup() const override final
    { return ${initialSetup}; }

    QScxmlExecutableContent::InstructionId *instructions() const override final
    { return theInstructions; }

    QScxmlExecutableContent::StringId *dataNames(int *count) const override final
    { *count = ${dataNameCount}; return dataIds; }

    QScxmlExecutableContent::EvaluatorInfo evaluatorInfo(QScxmlExecutableContent::EvaluatorId evaluatorId) const override final
    { Q_ASSERT(evaluatorId >= 0); Q_ASSERT(evaluatorId < ${evaluatorCount}); return evaluators[evaluatorId]; }

    QScxmlExecutableContent::AssignmentInfo assignmentInfo(QScxmlExecutableContent::EvaluatorId assignmentId) const override final
    { Q_ASSERT(assignmentId >= 0); Q_ASSERT(assignmentId < ${assignmentCount}); return assignments[assignmentId]; }

    QScxmlExecutableContent::ForeachInfo foreachInfo(QScxmlExecutableContent::EvaluatorId foreachId) const override final
    { Q_ASSERT(foreachId >= 0); Q_ASSERT(foreachId < ${foreachCount}); return foreaches[foreachId]; }

    QString string(QScxmlExecutableContent::StringId id) const override final
    {
        Q_ASSERT(id >= QScxmlExecutableContent::NoString); Q_ASSERT(id < ${stringCount});
        if (id == QScxmlExecutableContent::NoString) return QString();
        const auto dataOffset = strings.offsetsAndSize[id * 2];
        const auto dataSize = strings.offsetsAndSize[id * 2 + 1];
        return QString::fromRawData(reinterpret_cast<const QChar*>(&strings.stringdata[dataOffset]), dataSize);
    }

    const qint32 *stateMachineTable() const override final
    { return theStateMachineTable; }

    QScxmlInvokableServiceFactory *serviceFactory(int id) const override final;

    ${classname} &stateMachine;
    ${dataModelField}

    static QScxmlExecutableContent::ParameterInfo param(QScxmlExecutableContent::StringId name,
                                                        QScxmlExecutableContent::EvaluatorId expr,
                                                        QScxmlExecutableContent::StringId location)
    {
        QScxmlExecutableContent::ParameterInfo p;
        p.name = name;
        p.expr = expr;
        p.location = location;
        return p;
    }

    static QScxmlExecutableContent::InvokeInfo invoke(
            QScxmlExecutableContent::StringId id,
            QScxmlExecutableContent::StringId prefix,
            QScxmlExecutableContent::EvaluatorId expr,
            QScxmlExecutableContent::StringId location,
            QScxmlExecutableContent::StringId context,
            QScxmlExecutableContent::ContainerId finalize,
            bool autoforward)
    {
        QScxmlExecutableContent::InvokeInfo i;
        i.id = id;
        i.prefix = prefix;
        i.expr = expr;
        i.location = location;
        i.context = context;
        i.finalize = finalize;
        i.autoforward = autoforward;
        return i;
    }

    static qint32 theInstructions[];
    static QScxmlExecutableContent::StringId dataIds[];
    static QScxmlExecutableContent::EvaluatorInfo evaluators[];
    static QScxmlExecutableContent::AssignmentInfo assignments[];
    static QScxmlExecutableContent::ForeachInfo foreaches[];
    static const qint32 theStateMachineTable[];
    static struct Strings {
        const uint offsetsAndSize[${stringCount} * 2];
        char16_t stringdata[${stringdataSize}];
    } strings;
};

${classname}::${classname}(QObject *parent)
    : QScxmlStateMachine(&staticMetaObject, parent)
    , data(new Data(*this))
{ qRegisterMetaType<${classname} *>(); data->init(); }

${classname}::~${classname}()
{ delete data; }

QScxmlInvokableServiceFactory *${classname}::Data::serviceFactory(int id) const
{
${serviceFactories}
}

qint32 ${classname}::Data::theInstructions[] = {
${theInstructions}
};

QScxmlExecutableContent::StringId ${classname}::Data::dataIds[] = {
${dataIds}
};

QScxmlExecutableContent::EvaluatorInfo ${classname}::Data::evaluators[] = {
${evaluators}
};

QScxmlExecutableContent::AssignmentInfo ${classname}::Data::assignments[] = {
${assignments}
};

QScxmlExecutableContent::ForeachInfo ${classname}::Data::foreaches[] = {
${foreaches}
};

${classname}::Data::Strings ${classname}::Data::strings = {{
${strLits}
},{
${uniLits}
}};

const qint32 ${classname}::Data::theStateMachineTable[] = ${theStateMachineTable};

${metaObject}
