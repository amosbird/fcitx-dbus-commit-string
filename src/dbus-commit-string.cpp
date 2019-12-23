#include <dbus/dbus.h>
#include <fcitx/fcitx.h>
#include <fcitx/hook.h>
#include <fcitx/instance.h>
#include <fcitx/module.h>
#include <fcitx/module/dbus/dbusstuff.h>
#include <fcitx/module/dbus/fcitx-dbus.h>
#include <fcitx/module/ipc/ipc.h>

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

static void* DBusCommitStringCreate(FcitxInstance* instance);
static void DBusCommitStringDestroy(void* arg);

FCITX_EXPORT_API
FcitxModule module = { .Create = DBusCommitStringCreate,
    .SetFD = NULL,
    .ProcessEvent = NULL,
    .Destroy = DBusCommitStringDestroy,
    .ReloadConfig = NULL };

#define FCITX_COMMIT_STRING_DBUS_METHOD_NAME "CommitString"
#define FCITX_COMMIT_STRING_DBUS_PATH "/" FCITX_COMMIT_STRING_DBUS_METHOD_NAME

const char* introspection_xml
    = "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
      "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
      "<node name=\"" FCITX_COMMIT_STRING_DBUS_PATH "\">"
      "<interface name=\"" DBUS_INTERFACE_INTROSPECTABLE "\">"
      "<method name=\"Introspect\">"
      "<arg name=\"data\" direction=\"out\" type=\"s\"/>"
      "</method>"
      "</interface>"
      "<interface name=\"" FCITX_IM_DBUS_INTERFACE "\">"
      "<method name=\"" FCITX_COMMIT_STRING_DBUS_METHOD_NAME "\">"
      "<arg name=\"im\" direction=\"in\" type=\"s\"/>"
      "</method>"
      "</interface>"
      "</node>";

typedef struct _FcitxDBusCommitString {
    FcitxInstance* owner;
    DBusConnection* connection;
} FcitxDBusCommitString;

static DBusHandlerResult FcitxDBusCommitStringEventHandler(
    DBusConnection* connection, DBusMessage* msg, void* user_data);

static DBusObjectPathVTable fcitxDBusCommitStringVTable
    = { NULL, &FcitxDBusCommitStringEventHandler, NULL, NULL, NULL, NULL };

static void* DBusCommitStringCreate(FcitxInstance* instance) {
    void* userData = fcitx_utils_malloc0(sizeof(FcitxDBusCommitString));
    FcitxDBusCommitString* dbusCommitString = static_cast<FcitxDBusCommitString*>(userData);
    dbusCommitString->owner = instance;

    do {
        dbusCommitString->connection = FcitxDBusGetConnection(instance);
        if (!dbusCommitString->connection) {
            FcitxLog(ERROR, "DBus Not initialized");
            break;
        }

        dbus_bool_t succeeded = dbus_connection_register_object_path(dbusCommitString->connection,
            FCITX_COMMIT_STRING_DBUS_PATH, &fcitxDBusCommitStringVTable, userData);
        if (!succeeded) {
            FcitxLog(ERROR, "Failed to register " FCITX_COMMIT_STRING_DBUS_PATH);
            break;
        }

        return userData;
    } while (0);

    free(userData);
    return NULL;
}

static void DBusCommitStringDestroy(void* arg) {
    FcitxDBusCommitString* dbusCommitString = static_cast<FcitxDBusCommitString*>(arg);
    if (dbusCommitString && dbusCommitString->connection) {
        dbus_connection_unregister_object_path(
            dbusCommitString->connection, FCITX_COMMIT_STRING_DBUS_PATH);
    }
    free(dbusCommitString);
}

static DBusHandlerResult FcitxDBusCommitStringEventHandler(
    DBusConnection* connection, DBusMessage* message, void* userData) {
    FcitxDBusCommitString* dbusCommitString = static_cast<FcitxDBusCommitString*>(userData);
    FcitxInstance* instance = dbusCommitString->owner;
    DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    DBusMessage* reply = NULL;

    if (dbus_message_is_method_call(message, DBUS_INTERFACE_INTROSPECTABLE, "Introspect")) {
        reply = dbus_message_new_method_return(message);
        if (!reply)
            FcitxLog(ERROR, "Failed to allocate DBusMessage for reply!");
        else
            dbus_message_append_args(
                reply, DBUS_TYPE_STRING, &introspection_xml, DBUS_TYPE_INVALID);
    } else if (dbus_message_is_method_call(
                   message, FCITX_IM_DBUS_INTERFACE, FCITX_COMMIT_STRING_DBUS_METHOD_NAME)) {
        DBusError error;
        dbus_error_init(&error);
        char* imname = NULL;
        if (dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &imname, DBUS_TYPE_INVALID)) {
            FcitxInputContext* rec = FcitxInstanceGetCurrentIC(instance);
            if (rec) {
                FcitxInstanceCommitString(instance, rec, imname);
                FcitxLog(INFO, "Commit string: %s", imname);
            } else {
                FcitxLog(INFO, "Frondend is null, cannot commit string");
            }
            reply = dbus_message_new_method_return(message);
        } else {
            reply = dbus_message_new_error_printf(message, DBUS_ERROR_UNKNOWN_METHOD,
                "No such method with signature (%s)", dbus_message_get_signature(message));
        }
        dbus_error_free(&error);
    }

    if (reply) {
        dbus_connection_send(connection, reply, NULL);
        dbus_message_unref(reply);
        dbus_connection_flush(connection);
        result = DBUS_HANDLER_RESULT_HANDLED;
    }

    return result;
}
