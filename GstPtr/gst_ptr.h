/*
 *  GstPtr<Type> is a type of shared pointer specialized for GStreamer.
 *  (C) 2022 Nacho Garcia <nacho.garglez@gmail.com>
 *  License: https://www.gnu.org/licenses/lgpl-3.0.html LGPL version 3 or higher
 *
 *  This needs C++17
 *
 */

/*
GStreamer's functions return and expect different type of pointers, and it's not
possible to create full-automatic smart pointers without some tool to extract
the semantics from .gir files (GObject introspection files).
This is a light-weight header for those that want a C usage of GStreamer
from C++, but saving the trouble of dealing with raw pointers.

You need to check the library documentation to be sure about how to construct
and how to use this pointer.
Please read it carefully in order to avoid leaks or segfaults.

0. Quick guide
---------------

/!\ Always check GStreamer's documentation to verify what it returns/expects /!\

 Construct from a raw pointer

 +--------------------------+----------------------------------++
 | Function returns         |  Methods                          |
 +--------------------------+-----------------------------------+
 | [Transfer::full]         | GstPtr<Type>::GstPtr (Type *&&)   |
 |                          | GstPtr<Type>::GstPtr (Type *&)    |
 |                          | GstPtr<Type>& operator=(Type *&&) |
 |                          | GstPtr<Type>& operator=(Type *&)  |
 +--------------------------+-----------------------------------+
 | [Transfer::m_floating]     | GstPtr<Type>::GstPtr (Type *&&)   |
 |                          | GstPtr<Type>::GstPtr (Type *&)    |
 |                          | GstPtr<Type>& operator=(Type *&&) |
 |                          | GstPtr<Type>& operator=(Type *&)  |
 |                          |                                   |
 |                          |          AND THEN                 |
 |                          |                                   |
 |                          | GstPtr<Type>::sink                |
 +--------------------------+-----------------------------------|
 | [Transfer::none]         | GstPtr<Type>::transferNone (Type*)|
 |                          |                                   |
 +--------------------------+-----------------------------------+

 Pass to a parameter expecting a raw pointer

+----------------------+-------------------------------------------------------+
| Function expects     |  Methods                                              |
+----------------------+-------------------------------------------------------+
| self-reference       | Type* GstPtr<Type>::self()                            |
| [Transfer::none]     | BaseType* GstPtr<Type>::self<BaseType>()              |
|                      | DerivedType* GstPtr<Type>::selfDynamic<DerivedType>() |
+----------------------+-------------------------------------------------------+
| [Transfer::full]     | Type* GstPtr<Type>::transferFull()                    |
+----------------------+-------------------------------------------------------+

1) How to use
--------------
First, you want this type if you need the same functionality as std::shared_ptr.

If you just want a view of a pointer (i.e, you don't plan ref/unref it, and you
are not going to store the pointer), just use the GStreamer's raw pointer.

1.1 First check the function documentation
-------------------------------------------
It's critical to know if the callee transfers ownership to us, or it doesn't.
Using the incorrect method here will lead to a mem leak or double unref (=crash)

1.1.1 Check if the function returns [transfer::full] or [Transfer::m_floating]
----------------------------------------------------------------------------
In this case, function is transferring ownership. You can use
constructors or assignment operators. For example:

                            [transfer::m_floating]
GstPtr<GstElement> m_pipe = gst_pipeline_new(... )

Also, for a [transfer::m_floating] function, documentation states that
you should do:

m_pipe.sink()

In practice, a lot of functions returning [transfer::m_floating] do not actually
return a m_floating reference, but in this case the ::sink() method does nothing.
Best practice is calling it for all [transfer::m_floating] functions.

[transfer::full] functions work as expected, but do not call ::sink for those
because some functions returning [transfer::full] do not clear the m_floating
flag, thus causing leaks is ::sink is called.


1.1.2. Check if the function returns [transfer::none]
------------------------------------------------------
If the GStreamer's function returns [transfer::none], it is probably designed
for a temporal usage of the returned value (a pointer view).
In case you want to construct a GstPtr from this pointer (for example, to save
it for later usage), you need to use GstPtr<Type>::transferNone instead of the
constructors or assignment operators.

For example:

GstPtr<GParamSpec> m_param_spec;
                          <--- [transfer::none] (just a view)
m_param_spec.transferNone( g_object_class_find_property(... )

Taking ownership of a [transfer::none] by using constructors or assignment,
will result in an extra _unref and memory corruption.

2. How to operate with the constructed GstPtr<Type>
---------------------------------------------------
Use it as a std::shared_ptr. It will add refs and will remove refs as necessary.

 3. How to pass the pointer back to GStreamer
--------------------------------------------
Read the GStreamer's documentation again. You need to know if the function
expects taking ownership or not.

3.1. If the function expects [transfer::none] or it's
 a "self" parameter for referring the object (this is a very common case):
---------------------------------------------------------------------------
use GstPtr<Type>::self to pass the inner raw pointer. You can also
perform a static or dynamic cast to another type, i.e:

 GstPtr<GstBin> gst_bin; // a GstBin

 gst_bin.self<GstElement> ()  //Pass raw pointer casting it up to GstElement

- Passing the pointer as "self" to a parameter that expects taking ownership
  will result in a memory leak.


3.2. If the function expects [transfer::full]
---------------------------------------------

If a function's parameter is marked as [transfer::full], it will take ownership.
In this case, use GstPtr<Type>::transferFull() to "move" your pointer to
the function.
Your GstPtr will not be longer valid (nullptr) after transferring ownership.
This behavior is for safety and coherence.
If you are still wanting the pointer after passing ownership, just make a copy
and transfer the copy instead. i.e:

 auto copy_of_gst_bin = gst_bin;
 function_that_expects_full_transfer (copy_of_gst_bin.transferFull())

4. Static and dynamic casting
 ---------------------------------------------
 Use:

 Casting to a raw Ptr:

 gst_bin.self<BaseType> ()
 gst_bin.selfDynamic<DerivedType> ()

 Casting to another GstPtr<>:

 GstPtr<Base> staticGstPtrCast(GstPtr<Derived>)
 GstPtr<Derived> dynamicGstPtrCast(GstPtr<Base>)

 to cast static (to a base type), or dynamic (to a derived type).

 Static cast is checked at build-time.
 Dynamic cast use GLib's functions for casting, but it will throw std::bad_alloc
 if the cast can't be done. GLib's function instead, issue a warning.

 Casting between different GstPtr is done with the free functions
 staticGstPtrCast and dynamicGstPtrCast, for example:

 GstPtr<GstBin> asElement = dynamicGstPtrCast<GstBin>(m_pipe);

*/

#pragma once

#if __cplusplus < 201703L
#error Need C++17 or higher
#endif

#if __has_include("gst/gst.h")
#include <gst/gst.h>
#endif

#include <cassert>
#include <functional>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace detail {

//------------------------------------------------
// This is implementation-detail.
// You should take a look if some Type is missing
//------------------------------------------------

// We declare interfaces for different objects from here, describing
// what functions should be used for ref, unref, and sink.

// Struct naming convention is I+<Type>

struct UnimplementedInterface {};
template <typename> struct GetInterface {
  using type = UnimplementedInterface;
};

struct IGObject {
  template <typename T> static void ref(T *ptr) noexcept { g_object_ref(ptr); }
  template <typename T> static void unref(T *ptr) noexcept {
    g_object_unref(ptr);
  }
  template <typename T> static void sink(T *ptr) noexcept {
    g_object_ref_sink(ptr);
  }
};

// Other objects simply inherit for its corresponding base class according
// to the documentation.

struct IGstObject : IGObject {};
struct IGstElement : IGstObject {};
struct IGstPad : IGstObject {};
struct IGstBin : IGstElement {};
struct IGstPipeline : IGstBin {};
struct IGstBus : IGstObject {};

struct IGstCaps {
  template <typename T> static void ref(T *ptr) noexcept { gst_caps_ref(ptr); }
  template <typename T> static void unref(T *ptr) noexcept {
    gst_caps_unref(ptr);
  }
};

struct IGParamSpec {
  template <typename T> static void ref(T *ptr) noexcept {
    g_param_spec_ref(ptr);
  }
  template <typename T> static void unref(T *ptr) noexcept {
    g_param_spec_unref(ptr);
  }
};

struct IGMainLoop {
  template <typename T> static void ref(T *ptr) noexcept {
    g_main_loop_ref(ptr);
  }
  template <typename T> static void unref(T *ptr) noexcept {
    g_main_loop_unref(ptr);
  }
};

// Just suppressing clang-tidy: "consider using constexpr instead of macro"
// NOLINTNEXTLINE
#define GST_PTR_MAP_INTERFACE_WITH_TYPE(GstreamerType, Gtype_)                 \
  template <> struct GetInterface<GstreamerType> {                             \
    using type = I##GstreamerType;                                             \
    static GType getGType() { return Gtype_; };                                \
  };

// A missing type? Extend the definitions by creating this #include.
#if __has_include("gst_ptr_extend.h")
#include "gst_ptr_extend.h"
#endif

// Here we simply map every interface with its corresponding type
GST_PTR_MAP_INTERFACE_WITH_TYPE(GObject, G_TYPE_OBJECT)
GST_PTR_MAP_INTERFACE_WITH_TYPE(GstObject, GST_TYPE_OBJECT)
GST_PTR_MAP_INTERFACE_WITH_TYPE(GstElement, GST_TYPE_ELEMENT)
GST_PTR_MAP_INTERFACE_WITH_TYPE(GstBin, GST_TYPE_BIN)
GST_PTR_MAP_INTERFACE_WITH_TYPE(GstPipeline, GST_TYPE_PIPELINE)
GST_PTR_MAP_INTERFACE_WITH_TYPE(GstCaps, GST_TYPE_CAPS)
GST_PTR_MAP_INTERFACE_WITH_TYPE(GstBus, GST_TYPE_BUS)
GST_PTR_MAP_INTERFACE_WITH_TYPE(GMainLoop, G_TYPE_NONE)
GST_PTR_MAP_INTERFACE_WITH_TYPE(GParamSpec, G_TYPE_PARAM)
GST_PTR_MAP_INTERFACE_WITH_TYPE(GstPad, GST_TYPE_PAD)

template <typename T> struct IsInterfaceImplemented {
  static constexpr bool value =
      !std::is_same_v<typename GetInterface<T>::type, UnimplementedInterface>;
};

template <typename T>
constexpr auto hasSinkFunction_sfinae(int)
    -> decltype(GetInterface<T>::type::template sink<T>(nullptr), bool()) {
  return true;
}
template <typename T> constexpr auto hasSinkFunction_sfinae(long) {
  return false;
}
// - If the sink function exists, the first function will be compiled
// - If sink doesn't exist the first function will be discarded with SFINAE
// - The int/long parameters are just to avoid function redefinition
template <typename T> struct HasSinkFunction {
  static constexpr bool value = hasSinkFunction_sfinae<T>(int(0));
};

} // namespace detail

//--------------------------------------------------
// The GstPtr<  > object, finally.
//--------------------------------------------------
template <typename Type> struct GstPtr {

  static_assert(detail::IsInterfaceImplemented<Type>::value,
                "So sorry! There's no interface defined for this type. "
                "You need add it into GstPtr<> source code or extend "
                "this header");

  // GstPtr is nullptr by default
  GstPtr() noexcept = default;

  //--------------------------------------------------
  // Constructions from raw pointer
  //--------------------------------------------------

  // Constructs from R-Value, what means a temporal pointer returned by someone.

  GstPtr(Type *&&rawPointer) noexcept { reset(rawPointer); }

  // Constructs from L-Value, what means a pointer previously created.
  // In this case, this constructor will also set that pointer to nullptr
  // for safety. It is supposed that when you are creating a GstPtr you
  // won't manually handle the raw pointer anymore, so the pointer is nulled
  // here to prevent accidental use. It's easier to catch a nullptr access
  // than a memory leak or a memory corruption...

  GstPtr(Type *&rawPointer) noexcept {
    reset(rawPointer);
    *(std::addressof(rawPointer)) = nullptr;
  }

  // The assignments have the same semantics and explanations as the
  // corresponding constructors.

  GstPtr &operator=(Type *&&rawPointer) noexcept {
    reset(rawPointer);
    return *this;
  }

  GstPtr &operator=(Type *&rawPointer) noexcept {
    reset(rawPointer);
    *(std::addressof(rawPointer)) = nullptr;
    return *this;
  }

  //--------------------------------------------------
  // Constructors from another GstPtr< >
  //--------------------------------------------------

  GstPtr &operator=(const GstPtr &other) noexcept {
    if (this != &other) {
      takeReference(other);
    }
    return *this;
  }

  GstPtr &operator=(GstPtr &&other) noexcept {
    moveReference(std::move(other));
    return *this;
  }

  GstPtr(GstPtr &&other) noexcept { moveReference(std::move(other)); }
  GstPtr(const GstPtr &other) noexcept { takeReference(other); }

  /// Full-Transfers ("moves") this GstPtr< > into a function
  /// asking for [Transfer::full]
  /// GstPtr< > will be nullptr after this operation.

  Type *transferFull() noexcept {
    Type *toTransfer = m_pointer;
    m_pointer = nullptr;
    return toTransfer;
  }

  /// Takes a rawPointer pointer from a function
  /// documented as [Transfer::none]

  void transferNone(Type *rawPointer) noexcept {
    reset(rawPointer);
    if (m_pointer != nullptr) {
      detail::GetInterface<Type>::type::ref(m_pointer);
    }
  }
  /// Try to "sink" a m_floating reference.
  /// This is a weird GObject concept that tries to emulate move semantics,
  /// a "m_floating" object is a temporal object.
  ///
  /// An interface can implement a  "sink" function, what means:
  ///
  /// * If the object is "m_floating" (temporal object), the m_floating flag is
  /// deleted and the refcount does not change. Thus, we are taking ownership.
  ///
  /// * If the object is NOT m_floating means that someone is keeping a reference
  /// and will unref it. In this case, the sunk interface clears the m_floating
  /// flag *AND* adds a reference.

  // Note: this would be automatic, but I have found some GStreamer functions
  // returning [transfer:full] with the m_floating flag set.

  template <typename U = Type>
  typename std::enable_if<detail::HasSinkFunction<U>::value, void>::type
  sink() noexcept {
    detail::GetInterface<Type>::type::sink(m_pointer);
  }

  /// Pass the inner raw ptr to a function
  /// that is not supposed to take ownership.

  Type *self() const noexcept { return m_pointer; }

  /// Pass the inner raw ptr to a function
  /// that is not supposed to take ownership, using a
  /// safe cast to toBaseType*
  /// This is static cast, thus only casting to a base class is allowed.
  /// For dynamic casting, use selfDynamic<toDerivedType>().

  template <typename toBaseType> toBaseType *self() const noexcept {
    using BaseInterface = typename detail::GetInterface<toBaseType>::type;
    using DerivedInterface = typename detail::GetInterface<Type>::type;
    static_assert(std::is_base_of_v<BaseInterface, DerivedInterface>,
                  "For static casting, you can only cast to base objects. Use "
                  "selfDynamic< >");
    return (toBaseType *)(m_pointer);
  }

  /// Pass the inner raw ptr to a function
  /// that is not supposed to take ownership, using a
  /// dynamic cast to toDerivedType*
  /// std::bad_cast exception if no such cast is possible
  template <typename toDerivedType> toDerivedType *selfDynamic() const {
    using DerivedInterface = typename detail::GetInterface<toDerivedType>;
    auto mayPromote = g_type_check_instance_is_a((GTypeInstance *)m_pointer,
                                                 DerivedInterface::getGType());
    if (mayPromote != TRUE) {
      throw std::bad_cast();
    }
    return (toDerivedType *)g_type_check_instance_cast(
        (GTypeInstance *)m_pointer, DerivedInterface::getGType());
  }

  /// Dereference operators
  const Type *operator->() const noexcept { return m_pointer; }

  /// Returns true if GstPtr< > is not nullptr
  explicit operator bool() const noexcept { return m_pointer != nullptr; }

  ~GstPtr() { reset(nullptr); }

private:
  // Moves another GstPtr< > into this one.
  void moveReference(GstPtr &&other) noexcept {
    m_pointer = other.m_pointer;
    other.m_pointer = nullptr;
  }

  // Copies another GstPtr< > into this one, thus adds a ref
  void takeReference(const GstPtr &other) noexcept {
    reset(other.m_pointer);
    if (m_pointer != nullptr) {
      detail::GetInterface<Type>::type::ref(m_pointer);
    }
  }
  // Resets this GstPtr< > for containing another raw pointer.
  // Previous content in unref.
  void reset(Type *rawPointer) noexcept {
    if (m_pointer != nullptr) {
      detail::GetInterface<Type>::type::unref(m_pointer);
    }
    m_pointer = rawPointer;
  }

  Type *m_pointer = nullptr;
};

template <typename Base, typename Derived>
[[nodiscard]] inline GstPtr<Base> staticGstPtrCast(GstPtr<Derived> &derived) {
  GstPtr<Base> base;
  base.transferNone(derived.template self<Base>());
  return base;
}
template <typename Derived, typename Base>
[[nodiscard]] inline GstPtr<Derived> dynamicGstPtrCast(GstPtr<Base> &base) {
  GstPtr<Derived> derived;
  derived.transferNone(base.template selfDynamic<Derived>());
  return derived;
}
