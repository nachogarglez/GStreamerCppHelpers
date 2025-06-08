#include <gtest/gtest.h>

// Note: tests have to be run with valgrind, in order to catch leaks.

//
// Dummy glib/gstreamer
// It's not necessary using the real libraries for testing GstPtr<> functionality
//
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

// NOLINTNEXTLINE
GObject *g_function_full_transfer() {
    auto *newObject = new GObject();
    g_object_ref(newObject);
    return newObject;
}
// NOLINTNEXTLINE
GObject *g_function_float_transfer_floating() {
    auto *newObject = new GObject();
    newObject->m_floating=true;
    return newObject;
}


// NOLINTNEXTLINE
GstPipeline *g_function_full_transfer_pipeline() {
    auto *newObject = new GstPipeline();
    g_object_ref(newObject);
    return newObject;
}
// NOLINTNEXTLINE
GstCaps *g_function_full_transfer_caps() {
    auto *newObject = new GstCaps();
    gst_mini_object_ref(newObject);
    return newObject;
}

// NOLINTNEXTLINE
void g_function_get_full_transfer(GObject *object) {
    g_object_unref(object);
}

// NOLINTNEXTLINE
GObject *g_function_transfer_none() {
    auto *newObject = new GObject();
    g_object_ref(newObject);
    return newObject;
}
// NOLINTNEXTLINE
void g_function_transfer_none_release(GObject *object) {
    g_object_unref(object);
}
// NOLINTNEXTLINE
void g_function_get_self_gst_object(GObject *object) {
}

// NOLINTNEXTLINE
void g_function_get_self_pipeline(GstPipeline *object) {
}
// NOLINTNEXTLINE
bool g_type_check_instance_is_a(GTypeInstance * m_pointer,GType type){
    if (type==GST_TYPE_PIPELINE){
        GstPipeline* pipeline = nullptr;
        try {
            pipeline = dynamic_cast<GstPipeline *>(m_pointer);
        }catch (...){}
        return pipeline!= nullptr;
    }
    std::abort();
}
// NOLINTNEXTLINE
GTypeInstance * g_type_check_instance_cast(GTypeInstance * m_pointer,GType type){
    return m_pointer;
}

//
// The tests, finally.
//

#include "../gst_ptr.h"

TEST(GstPtr, constructor_default) {
    GstPtr<GstObject> gstObject;
    ASSERT_EQ(gstObject.self(), nullptr);
}

TEST(GstPtr, constructor_full_transfer_r_value) {
    GstPtr<GObject> gstObject = g_function_full_transfer();
    ASSERT_EQ(gstObject.self()->m_refCount, 1);
}

TEST(GstPtr, constructor_full_transfer_l_value) {
    auto *pointer = g_function_full_transfer();
    GstPtr<GObject> gstObject{pointer};
    ASSERT_EQ(pointer, nullptr);
    ASSERT_EQ(gstObject.self()->m_refCount, 1);
}

TEST(GstPtr, constructor_assignament_full_transfer_r_value) {
    GstPtr<GObject> gstObject;
    gstObject = g_function_full_transfer();
    ASSERT_EQ(gstObject.self()->m_refCount, 1);
}

TEST(GstPtr, constructor_assignament_full_transfer_l_value) {
    auto *pointer = g_function_full_transfer();
    GstPtr<GObject> gstObject;
    gstObject = pointer;
    ASSERT_EQ(pointer, nullptr);
    ASSERT_EQ(gstObject.self()->m_refCount, 1);
}

TEST(GstPtr, constructor_from_transfer_none) {
    GstPtr<GObject> gstObject;
    gstObject.transferNone(g_function_transfer_none());
    ASSERT_EQ(gstObject.self()->m_refCount, 2);
    g_function_transfer_none_release(gstObject.self());
    ASSERT_EQ(gstObject.self()->m_refCount, 1);
}

TEST(GstPtr, constructor_float_r_value) {
    GstPtr<GObject> gstObject = g_function_float_transfer_floating();
    gstObject.sink();
    ASSERT_EQ(gstObject.self()->m_refCount, 1);
}


TEST(GstPtr, copy_constructor) {
    GstPtr<GObject> objA = g_function_full_transfer();
    GstPtr<GObject> objB{objA};
    GstPtr<GObject> objC{objA};
    ASSERT_EQ(objA.self()->m_refCount, 3);
    ASSERT_EQ(objB.self()->m_refCount, 3);
    ASSERT_EQ(objC.self()->m_refCount, 3);
}

TEST(GstPtr, copy_assignament) {
    GstPtr<GObject> objA = g_function_full_transfer();
    GstPtr<GObject> objB;
    objB = objA;
    GstPtr<GObject> objC;
    objC = objA;
    ASSERT_EQ(objA.self()->m_refCount, 3);
    ASSERT_EQ(objB.self()->m_refCount, 3);
    ASSERT_EQ(objC.self()->m_refCount, 3);
}

TEST(GstPtr, copy_re_assignament) {
    GstPtr<GObject> objA = g_function_full_transfer();
    GstPtr<GObject> objB = g_function_full_transfer();
    objB = objA;
    ASSERT_EQ(objA.self()->m_refCount, 2);
    ASSERT_EQ(objB.self()->m_refCount, 2);
}

TEST(GstPtr, move_constructor) {
    GstPtr<GObject> obj = g_function_full_transfer();
    GstPtr<GObject> moved (std::move(obj));
    ASSERT_EQ(moved.self()->m_refCount, 1);
    ASSERT_EQ(obj.self(), nullptr);
}

TEST(GstPtr, move_assignament) {
    GstPtr<GObject> obj = g_function_full_transfer();
    GstPtr<GObject> moved;
    moved = std::move(obj);
    ASSERT_EQ(moved.self()->m_refCount, 1);
    ASSERT_EQ(obj.self(), nullptr);
}

TEST(GstPtr, pass_transfer_full) {
    GstPtr<GObject> obj = g_function_full_transfer();
    g_function_get_full_transfer(obj.transferFull());
    ASSERT_EQ(obj.self(), nullptr);
}

//Static test. Pass if it compiles.
TEST(GstPtr, self_static_cast) {
    GstPtr<GstPipeline> obj = g_function_full_transfer_pipeline();
    g_function_get_self_gst_object(obj.self<GstObject>());
}

TEST(GstPtr, self_dynamic_cast_sucess) {
    GstPtr<GObject> obj =  g_function_full_transfer_pipeline();
    g_function_get_self_pipeline(obj.selfDynamic<GstPipeline>());

}

TEST(GstPtr, self_dynamic_cast_fail) {
    GstPtr<GstCaps> caps = g_function_full_transfer_caps();
    ASSERT_THROW(g_function_get_self_pipeline(caps.selfDynamic<GstPipeline>()),
                 std::bad_cast);
}

TEST(GstPtr, static_cast_between_gstptr) {
    GstPtr<GstPipeline> pipe = g_function_full_transfer_pipeline();
    GstPtr<GstObject> obj= staticGstPtrCast<GstObject>(pipe);
    ASSERT_EQ(pipe.self()->m_refCount, 2);
    ASSERT_EQ(obj.self()->m_refCount, 2);
}

TEST(GstPtr, dynamic_cast_between_gstptr_sucess) {
    GstPtr<GObject> obj=  g_function_full_transfer_pipeline();
    GstPtr<GstPipeline> pipe = dynamicGstPtrCast<GstPipeline>(obj);
    ASSERT_EQ(pipe.self()->m_refCount, 2);
    ASSERT_EQ(obj.self()->m_refCount, 2);
}

TEST(GstPtr, dynamic_cast_between_gstptr_fail) {
    GstPtr<GstCaps> obj=  g_function_full_transfer_caps();
    GstPtr<GstPipeline> pipe;
    ASSERT_THROW(pipe= dynamicGstPtrCast<GstPipeline>(obj),std::bad_cast);
}

TEST(GstPtr, bool_operator) {
    GstPtr<GstCaps> obj;
    ASSERT_EQ((bool)obj,false);
    obj=  g_function_full_transfer_caps();
    ASSERT_EQ((bool)obj,true);
}

TEST(GstPtr, dereference_operator) {
    GstPtr<GstCaps> obj= g_function_full_transfer_caps();
    ASSERT_EQ(obj->m_dummy,0x69);
}
