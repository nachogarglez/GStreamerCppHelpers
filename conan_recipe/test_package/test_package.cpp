//
// Dummy glib/gstreamer
// It's not necessary using the real libraries for testing GstPtr<> functionality
//
#include <cassert>
using GType = long;
constexpr GType G_TYPE_OBJECT = 0x01;
constexpr GType GST_TYPE_OBJECT = 0x02;
constexpr GType GST_TYPE_ELEMENT = 0x03;
constexpr GType GST_TYPE_BIN = 0x04;
constexpr GType GST_TYPE_PIPELINE = 0x05;
constexpr GType GST_TYPE_CAPS = 0x06;
constexpr GType GST_TYPE_BUS = 0x07;
constexpr GType G_TYPE_NONE = 0x08;
constexpr GType G_TYPE_PARAM = 0x09;
constexpr GType GST_TYPE_PAD = 0x0A;
constexpr GType GST_TYPE_BUFFER = 0x0B;
constexpr GType GST_TYPE_EVENT = 0x0C;
constexpr GType GST_TYPE_CONTEXT = 0x0D;
constexpr bool TRUE = true;

struct GTypeInstance {
    virtual ~GTypeInstance() = default;
    virtual void ref() {
        m_refCount++;
    }
    virtual void unref() {
        m_refCount--;
    }
    virtual void sink() {
        if (m_floating) {
            m_refCount++;
        }
    }
    const int m_dummy=0x69;
    long m_refCount = 0;
    bool m_floating=false;
};
struct GObject : public GTypeInstance {};
struct GstObject : public GObject {};
struct GstElement : public GObject {};
struct GstPad : public GstObject {};
struct GstBin : public GstElement {};
struct GstPipeline : public GstBin {};
struct GstBus : public GstObject {};
class GstMiniObject : public GTypeInstance {};
class GstCaps : public GstMiniObject {};
class GstBuffer : public GstMiniObject {};
class GstEvent : public GstMiniObject {};
class GstContext : public GstMiniObject {};
struct GParamSpec : public GTypeInstance {};
struct GMainLoop : public GTypeInstance {};

// NOLINTNEXTLINE
void g_object_unref(GObject *obj) {
    obj->unref();
    assert (obj->m_refCount >= 0);
    if (obj->m_refCount == 0) {
        delete obj;
    }
}

// NOLINTNEXTLINE
void g_object_ref(GObject *obj) {
    obj->ref();
}

// NOLINTNEXTLINE
void g_object_ref_sink(GObject *obj) {
    obj->sink();
}


// NOLINTNEXTLINE
void gst_mini_object_unref(GstMiniObject *obj) {
    obj->unref();
    assert (obj->m_refCount >= 0);
    if (obj->m_refCount == 0) {
        delete obj;
    }
}
// NOLINTNEXTLINE
void gst_mini_object_ref(GstMiniObject *obj) {
    obj->ref();
}

#include <GstPtr/gst_ptr.h>
int main() {
    return 0;
}
