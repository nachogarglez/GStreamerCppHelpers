- [GstPtr < >](#gstptr--)
  - [Quick guide](#quick-guide)
    - [Construct from a raw pointer](#construct-from-a-raw-pointer)
    - [Pass to a parameter expecting a raw pointer](#pass-to-a-parameter-expecting-a-raw-pointer)
  - [How to use](#how-to-use)
    - [First check the function documentation](#first-check-the-function-documentation)
      - [Check if the function returns `[transfer::full]` or `[Transfer::floating]`](#check-if-the-function-returns-transferfull-or-transferfloating)
      - [Check if the function returns `[transfer::none]`](#check-if-the-function-returns-transfernone)
    - [How to operate with the constructed `GstPtr<Type>`](#how-to-operate-with-the-constructed-gstptrtype)
      - [How to pass the pointer back to GStreamer](#how-to-pass-the-pointer-back-to-gstreamer)
        - [If the function expects `[transfer::none]` or it's  a  `self` parameter for referring the object (this is a very common case):](#if-the-function-expects-transfernone-or-its--a--self-parameter-for-referring-the-object-this-is-a-very-common-case)
        - [If the function expects `[transfer::full]`](#if-the-function-expects-transferfull)
    - [Static and dynamic casting](#static-and-dynamic-casting)

# GstPtr < >

GStreamer's functions return and expect different type of pointers, and it's not
possible to create full-automatic smart pointers without some tool to extract
the semantics from .gir files (GObject introspection files).
This is a light-weight header for those that want a C usage of GStreamer
from C++, but saving the trouble of dealing with raw pointers.

You need to check the library documentation to be sure about how to construct
and how to use this pointer.
Please read it carefully in order to avoid leaks or segfaults.

##  Quick guide


 ⚠  Always check GStreamer's documentation to verify what it returns/expects  ⚠ 


 ### Construct from a raw pointer
 
 | Function returns         | Methods                                                                                                                                                                           |
|--------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 | [Transfer::full]         | GstPtr<Type>::GstPtr (Type *&&)<br>GstPtr<Type>::GstPtr (Type *&)<br>GstPtr<Type>& operator=(Type *&&)<br>GstPtr<Type>& operator=(Type *&)                                        |
 | [Transfer::floating]     | GstPtr<Type>::GstPtr (Type *&&)<br>GstPtr<Type>::GstPtr (Type *&)<br>GstPtr<Type>& operator=(Type *&&)<br>GstPtr<Type>& operator=(Type *&)<br>*AND THEN*<br>GstPtr<Type>::sink    |
 | [Transfer::none]         | GstPtr<Type>::transferNone (Type*)                                                                                                                                                |
 
 ###  Pass to a parameter expecting a raw pointer


| Function expects                    | Methods                                                                                                                         |
|-------------------------------------|---------------------------------------------------------------------------------------------------------------------------------|
| self-reference<br> [Transfer::none] | Type* GstPtr<Type>::self()<br>BaseType* GstPtr<Type>::self<BaseType>()<br>DerivedType* GstPtr<Type>::selfDynamic<DerivedType>() |
| [Transfer::full]                    | Type* GstPtr<Type>::transferFull()                                                                                              |


## How to use

First, you want this type if you need the same functionality as `std::shared_ptr`.

If you just want a view of a pointer (i.e, you don't plan ref/unref it, and you
are not going to store the pointer), just use the GStreamer's raw pointer.

### First check the function documentation

It's critical to know if the callee transfers ownership to us, or it doesn't.
Using the incorrect method here will lead to a mem leak or double unref (=crash)

####  Check if the function returns `[transfer::full]` or `[Transfer::floating]`

In this case, function is transferring ownership. You can use
constructors or assignment operators. For example:
```C++
                            [transfer::floating]
GstPtr<GstElement> m_pipe = gst_pipeline_new(... )
```

Also, for a `[transfer::floating]` function, documentation states that
you should do:

```C++
m_pipe.sink()
```

In practice, a lot of functions returning `[transfer::floating]` do not actually
return a `floating` reference, but in this case the `::sink()` method does nothing.
Best practice is calling it for all `[transfer::floating]` functions.

`[transfer::full]` functions work as expected, but do not call ::sink for those
because some functions returning `[transfer::full]` do not clear the m_floating
flag, thus causing leaks is `::sink` is called.


####  Check if the function returns `[transfer::none]`

If the GStreamer's function returns `[transfer::none]`, it is probably designed
for a temporal usage of the returned value (a pointer view).
In case you want to construct a GstPtr from this pointer (for example, to save
it for later usage), you need to use `GstPtr<Type>::transferNone` instead of the
constructors or assignment operators.

For example:

```c++
GstPtr<GParamSpec> m_param_spec;
                          <--- [transfer::none] (just a view)
m_param_spec.transferNone( g_object_class_find_property(... )
```

Taking ownership of a `[transfer::none]` by using constructors or assignment,
will result in an extra `_unref` and memory corruption.

###  How to operate with the constructed `GstPtr<Type>`
Use it as a `std::shared_ptr`. It will add refs and will remove refs as necessary.

#### How to pass the pointer back to GStreamer
Read the GStreamer's documentation again. You need to know if the function
expects taking ownership or not.

##### If the function expects `[transfer::none]` or it's  a  `self` parameter for referring the object (this is a very common case):

use `GstPtr<Type>::self` to pass the inner raw pointer. You can also
perform a static or dynamic cast to another type, i.e:

```c++
 GstPtr<GstBin> gst_bin; // a GstBin
 gst_bin.self<GstElement> ()  //Pass raw pointer casting it up to GstElement
 ```

- Passing the pointer as `self` to a parameter that expects taking ownership
  will result in a memory leak.


##### If the function expects `[transfer::full]`

If a function's parameter is marked as `[transfer::full]`, it will take ownership.
In this case, use `GstPtr<Type>::transferFull()` to "move" your pointer to
the function.
Your GstPtr will not be longer valid (`nullptr`) after transferring ownership.
This behavior is for safety and coherence.
If you are still wanting the pointer after passing ownership, just make a copy
and transfer the copy instead. i.e:

```c++
 auto copy_of_gst_bin = gst_bin;
 function_that_expects_full_transfer (copy_of_gst_bin.transferFull())
```

###  Static and dynamic casting

You can cast `self` either statically or dinamically, for example:

```c++
 gst_bin.self<BaseType> ()
 gst_bin.selfDynamic<DerivedType> ()
```

Free functions to casting to another `GstPtr<>`:

```c++
 GstPtr<Base> staticGstPtrCast(GstPtr<Derived>)
 GstPtr<Derived> dynamicGstPtrCast(GstPtr<Base>)
``` 

 
 * Static cast is checked at build-time.
 * Dynamic cast use GLib's functions for casting, but it will throw std::bad_alloc
 if the cast can't be done. GLib's function instead, issue a warning.

 Example:

```c++
GstPtr<GstBin> asElement = dynamicGstPtrCast<GstBin>(m_pipe);
```

