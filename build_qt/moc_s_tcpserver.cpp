/****************************************************************************
** Meta object code from reading C++ file 's_tcpserver.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../pro_ffmpeg_qt/s_tcpserver/s_tcpserver.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 's_tcpserver.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_S_TcpServer_t {
    QByteArrayData data[11];
    char stringdata0[146];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_S_TcpServer_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_S_TcpServer_t qt_meta_stringdata_S_TcpServer = {
    {
QT_MOC_LITERAL(0, 0, 11), // "S_TcpServer"
QT_MOC_LITERAL(1, 12, 14), // "readDataSignal"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 22), // "readDataFromPeerSignal"
QT_MOC_LITERAL(4, 51, 2), // "ip"
QT_MOC_LITERAL(5, 54, 4), // "port"
QT_MOC_LITERAL(6, 59, 16), // "disConnectSignal"
QT_MOC_LITERAL(7, 76, 26), // "newConnectIpPortInfoSignal"
QT_MOC_LITERAL(8, 103, 14), // "newConnectSlot"
QT_MOC_LITERAL(9, 118, 14), // "disconnectSlot"
QT_MOC_LITERAL(10, 133, 12) // "readDataSlot"

    },
    "S_TcpServer\0readDataSignal\0\0"
    "readDataFromPeerSignal\0ip\0port\0"
    "disConnectSignal\0newConnectIpPortInfoSignal\0"
    "newConnectSlot\0disconnectSlot\0"
    "readDataSlot"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_S_TcpServer[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x06 /* Public */,
       3,    3,   52,    2, 0x06 /* Public */,
       6,    2,   59,    2, 0x06 /* Public */,
       7,    2,   64,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       8,    0,   69,    2, 0x0a /* Public */,
       9,    0,   70,    2, 0x0a /* Public */,
      10,    0,   71,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void, QMetaType::QString, QMetaType::UShort, QMetaType::QString,    4,    5,    2,
    QMetaType::Void, QMetaType::QString, QMetaType::UShort,    4,    5,
    QMetaType::Void, QMetaType::QString, QMetaType::UShort,    4,    5,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void S_TcpServer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<S_TcpServer *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->readDataSignal((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: _t->readDataFromPeerSignal((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2])),(*reinterpret_cast< QString(*)>(_a[3]))); break;
        case 2: _t->disConnectSignal((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2]))); break;
        case 3: _t->newConnectIpPortInfoSignal((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2]))); break;
        case 4: _t->newConnectSlot(); break;
        case 5: _t->disconnectSlot(); break;
        case 6: _t->readDataSlot(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (S_TcpServer::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&S_TcpServer::readDataSignal)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (S_TcpServer::*)(QString , quint16 , QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&S_TcpServer::readDataFromPeerSignal)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (S_TcpServer::*)(QString , quint16 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&S_TcpServer::disConnectSignal)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (S_TcpServer::*)(QString , quint16 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&S_TcpServer::newConnectIpPortInfoSignal)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject S_TcpServer::staticMetaObject = { {
    QMetaObject::SuperData::link<QThread::staticMetaObject>(),
    qt_meta_stringdata_S_TcpServer.data,
    qt_meta_data_S_TcpServer,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *S_TcpServer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *S_TcpServer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_S_TcpServer.stringdata0))
        return static_cast<void*>(this);
    return QThread::qt_metacast(_clname);
}

int S_TcpServer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void S_TcpServer::readDataSignal(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void S_TcpServer::readDataFromPeerSignal(QString _t1, quint16 _t2, QString _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void S_TcpServer::disConnectSignal(QString _t1, quint16 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void S_TcpServer::newConnectIpPortInfoSignal(QString _t1, quint16 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
