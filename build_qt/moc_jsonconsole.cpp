/****************************************************************************
** Meta object code from reading C++ file 'jsonconsole.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../pro_ffmpeg_qt/jsonConsole/jsonconsole.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QVector>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'jsonconsole.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_JsonConsole_t {
    QByteArrayData data[25];
    char stringdata0[196];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_JsonConsole_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_JsonConsole_t qt_meta_stringdata_JsonConsole = {
    {
QT_MOC_LITERAL(0, 0, 11), // "JsonConsole"
QT_MOC_LITERAL(1, 12, 15), // "updateWinSignal"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 2), // "id"
QT_MOC_LITERAL(4, 32, 3), // "flg"
QT_MOC_LITERAL(5, 36, 17), // "setGeometrySignal"
QT_MOC_LITERAL(6, 54, 1), // "x"
QT_MOC_LITERAL(7, 56, 1), // "y"
QT_MOC_LITERAL(8, 58, 1), // "w"
QT_MOC_LITERAL(9, 60, 1), // "h"
QT_MOC_LITERAL(10, 62, 12), // "tcpAckSignal"
QT_MOC_LITERAL(11, 75, 2), // "ip"
QT_MOC_LITERAL(12, 78, 4), // "port"
QT_MOC_LITERAL(13, 83, 13), // "displaySignal"
QT_MOC_LITERAL(14, 97, 4), // "show"
QT_MOC_LITERAL(15, 102, 12), // "rebootSignal"
QT_MOC_LITERAL(16, 115, 14), // "pushFlowSignal"
QT_MOC_LITERAL(17, 130, 15), // "setMosaicSignal"
QT_MOC_LITERAL(18, 146, 4), // "rows"
QT_MOC_LITERAL(19, 151, 7), // "columns"
QT_MOC_LITERAL(20, 159, 5), // "width"
QT_MOC_LITERAL(21, 165, 6), // "height"
QT_MOC_LITERAL(22, 172, 4), // "freq"
QT_MOC_LITERAL(23, 177, 12), // "QVector<int>"
QT_MOC_LITERAL(24, 190, 5) // "gpuid"

    },
    "JsonConsole\0updateWinSignal\0\0id\0flg\0"
    "setGeometrySignal\0x\0y\0w\0h\0tcpAckSignal\0"
    "ip\0port\0displaySignal\0show\0rebootSignal\0"
    "pushFlowSignal\0setMosaicSignal\0rows\0"
    "columns\0width\0height\0freq\0QVector<int>\0"
    "gpuid"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_JsonConsole[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       9,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   59,    2, 0x06 /* Public */,
       1,    1,   64,    2, 0x26 /* Public | MethodCloned */,
       1,    0,   67,    2, 0x26 /* Public | MethodCloned */,
       5,    4,   68,    2, 0x06 /* Public */,
      10,    3,   77,    2, 0x06 /* Public */,
      13,    1,   84,    2, 0x06 /* Public */,
      15,    0,   87,    2, 0x06 /* Public */,
      16,    0,   88,    2, 0x06 /* Public */,
      17,    7,   89,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    3,    4,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int,    6,    7,    8,    9,
    QMetaType::Void, QMetaType::QString, QMetaType::UShort, QMetaType::QString,   11,   12,    2,
    QMetaType::Void, QMetaType::Bool,   14,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, 0x80000000 | 23, 0x80000000 | 23,   18,   19,   20,   21,   22,   24,   12,

       0        // eod
};

void JsonConsole::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<JsonConsole *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->updateWinSignal((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 1: _t->updateWinSignal((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->updateWinSignal(); break;
        case 3: _t->setGeometrySignal((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 4: _t->tcpAckSignal((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2])),(*reinterpret_cast< QString(*)>(_a[3]))); break;
        case 5: _t->displaySignal((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->rebootSignal(); break;
        case 7: _t->pushFlowSignal(); break;
        case 8: _t->setMosaicSignal((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< int(*)>(_a[5])),(*reinterpret_cast< QVector<int>(*)>(_a[6])),(*reinterpret_cast< QVector<int>(*)>(_a[7]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 8:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 6:
            case 5:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QVector<int> >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (JsonConsole::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&JsonConsole::updateWinSignal)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (JsonConsole::*)(int , int , int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&JsonConsole::setGeometrySignal)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (JsonConsole::*)(QString , quint16 , QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&JsonConsole::tcpAckSignal)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (JsonConsole::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&JsonConsole::displaySignal)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (JsonConsole::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&JsonConsole::rebootSignal)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (JsonConsole::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&JsonConsole::pushFlowSignal)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (JsonConsole::*)(int , int , int , int , int , QVector<int> , QVector<int> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&JsonConsole::setMosaicSignal)) {
                *result = 8;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject JsonConsole::staticMetaObject = { {
    QMetaObject::SuperData::link<QThread::staticMetaObject>(),
    qt_meta_stringdata_JsonConsole.data,
    qt_meta_data_JsonConsole,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *JsonConsole::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *JsonConsole::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_JsonConsole.stringdata0))
        return static_cast<void*>(this);
    return QThread::qt_metacast(_clname);
}

int JsonConsole::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void JsonConsole::updateWinSignal(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 3
void JsonConsole::setGeometrySignal(int _t1, int _t2, int _t3, int _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void JsonConsole::tcpAckSignal(QString _t1, quint16 _t2, QString _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void JsonConsole::displaySignal(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void JsonConsole::rebootSignal()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void JsonConsole::pushFlowSignal()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void JsonConsole::setMosaicSignal(int _t1, int _t2, int _t3, int _t4, int _t5, QVector<int> _t6, QVector<int> _t7)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t5))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t6))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t7))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
