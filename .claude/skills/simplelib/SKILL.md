---
name: simplelib
description: Brad Robinson's (Topten Software) header-only C++17 utility library "SimpleLib" - String/StringBuilder, List/Map/Set, OwnedPtr/SharedPtr/RefCountedPtr, Delegate, lock-free queues/stacks, Atomic, Mutex/SlimLock/SpinLock, Thread, CowList, AtomicPtrTransfer, Stream/FileStream/MemoryStream, Path, FileSystem, Geo (Vector/Rectangle/Affine/Matrix4/ColorF), JSON stringify and a tiny unit-test framework. Use whenever code includes SimpleLib headers (eg "../SimpleLib/Core.h", "Threading.h", "UnitTesting.h"), uses `namespace SimpleLib`, or is in a project with a SimpleLib submodule (Cantabile etc) - prefer SimpleLib types over the STL there.
---

# SimpleLib

Header-only C++17 library (`github.com/toptensoftware/SimpleLib`). No STL containers, no exceptions (except the test framework), no iterators. Pascal-case, .NET-flavoured API. Everything is in `namespace SimpleLib` **except `Delegate`, which is global**.

Full per-class API: see [reference.md](reference.md) in this folder. Read it before using a class you haven't used in this session. If the library source is available (usually a `SimpleLib/` submodule), the headers are short and well commented - check them when in doubt.

## Using it in a project

- Usually a git submodule at `<repo>/SimpleLib`, included by relative path or via an include path pointing at it:
  ```cpp
  #include "../SimpleLib/Core.h"        // strings, collections, smart ptrs, misc
  #include "../SimpleLib/Threading.h"   // atomics, locks, queues, Thread, CowList...
  #include "../SimpleLib/Stream.h"      // Stream, FileStream, MemoryStream
  #include "../SimpleLib/Path.h"
  #include "../SimpleLib/FileSystem.h"
  #include "../SimpleLib/Geo.h"
  #include "../SimpleLib/Json.h"
  using namespace SimpleLib;
  ```
  Use the PascalCase file names (they were normalized; old code may still use lowercase - fine on Windows, breaks on Linux).
- Requires C++17. MSVC needs `/EHsc` and usually `_CRT_SECURE_NO_WARNINGS`. Windows build auto-links `synchronization.lib`.
- `Platform/Win.h` includes `<Windows.h>` (and `#undef Yield`). Threading/Stream/FileSystem/Path pull it in.
- **Platform support:** Windows is complete. `Platform/Lin.h` only implements atomics + futex wait/wake - so on Linux only `Atomic`, `AtomicSemaphore`, the lock-free queues/stacks etc work; Thread/Mutex/SlimLock/Semaphore/ThreadLocal/FileStream/FileSystem/Path::GetCurrentDirectory are Windows-only.
- Projects build with Brad's `mk` tool (`*.mk.js` files: `this.use("c-cpp")`, `this.set({ includePath, define, msvc_cl_args, ... })`).

## Core conventions (get these right)

**Naming:** `GetCount()` not `size()`, `Add` not `push_back`, `IsEmpty()`, `GetAt(i)`, `TryXxx(out)` returns bool, `-1` for not-found indices. Sizes/indices are `int`.

**Iteration** - no `begin()/end()`, so no range-for. Use the iterator pattern:
```cpp
for (int i = 0; i < list.GetCount(); i++) use(list[i]);
for (auto it = list.Iterate(); it.Next(); ) use(it.Get());            // List, Set
for (auto it = map.Iterate(); it.Next(); ) use(it.GetKey(), it.GetValue());
```
Map/Set iterators assert if the collection is modified during iteration. `Map::Iterate()` is non-const.

**Collections are move-only.** `List`, `Map`, `Set` delete copy ctor/assign. Return them by value (moves) or pass by reference; never write `List<T> b = a;`.

**Element storage:** elements are copy-constructed in place (placement new from `const T&`) and **relocated with `memmove`** - element types must be copyable and trivially relocatable (no self-pointers, no types that register their own address).

**Ownership via semantics:** a container of `OwnedPtr<T>` owns its objects:
```cpp
List<OwnedPtr<Foo>> items;
items.Add(new Foo());        // takes ownership; TArg is Foo*
Foo* f = items[0];           // get raw pointer
items.RemoveAt(0);           // deletes the Foo
Foo* kept = items.DetachAt(0); // removes WITHOUT deleting
Map<String, OwnedPtr<Foo>> byName;  // same for map values
```

**Error codes, not exceptions:** Stream/FileSystem functions return `int` - `0` on success, else an errno-style code. Propagate with `RIFE(expr)` ("return if error"): `RIFE(file.Open(path, "rb"));`.

## String

`String` = `StringCore<char>` (also `WString`, `String32`). Immutable, ref-counted, cheap to copy.

```cpp
String s = "hello";
String t = String::Format("%s=%i (%.2f)", name.sz(), n, x);  // own printf impl, same on all platforms
if (s.StartsWith("he") && s.IndexOf("ll") >= 0) {}
if (s.IsEqualTo<SCaseI>("HELLO")) {}       // case-insensitive variants via template arg
List<String> parts; s.Split(",;", false, parts);
String j = String::Join(parts, ", ");
String u = s.Trim().ToUpper();
```

Pitfalls:
- **Null vs empty:** a default `String` is *null* - `sz()` returns `nullptr`, `GetLength()` returns 0. `IsNull()`, `IsEmpty()`, `!s` (null-or-empty), `String::IsNullOrEmpty(psz)`. Guard before passing `sz()` to C APIs like `strlen`/`fopen`.
- **Pass `.sz()` to `Format`/varargs**, never a `String` object.
- `SubString`, `ToUpper`, `ToLower`, `Replace`, `operator[]` are **non-const** methods - you can't call them on a `const String&` (copy it first: `String(s).SubString(...)`).
- Implicitly converts to `const char*`; there is no `operator+(const char*)` - use `s + String("x")`, `s += "x"`, or a `StringBuilder`.
- Comparison operators are case-sensitive.

`StringBuilder<char>` for building: `Append(ch|psz|psz,len)`, `Append(ch, count)`, `Format(...)`, `GetLength()`, `Truncate(n)`, `Clear()`, `ToString()`, `sz()`. `String s = sb;` and `return sb;` from a `String` function both work.

UTF conversion: `Encode<TTo>(const TFrom*)`, e.g. `WString w = Encode<wchar_t>(utf8);`, `String u8 = Encode<char>(wide);`.

## Collections quick reference

- `List<T>` - dynamic array; also stack (`Push/Pop/Tail`) and queue (`Enqueue/Dequeue/Head`). `Add` returns the index. `IndexOf/Contains/Remove(val)`, `InsertAt`, `RemoveAt(i[,count])`, `Move(from,to)`, `Swap`, `Sort()` / `Sort(cmp)`, `BinarySearch(index, key, cmp)`, `Filter(pred)`, `Map<TResult>(fn)` (explicit result type required), `AddRange(list)`, `GetBuffer()`.
- `Map<K,V,TKeyCompare=SDefaultCompare>` - hash map. `Add` (no-op if key exists), `Set` (insert or replace), `Get(k)` (asserts if missing), `Get(k, default)`, `TryGetValue(k, out)`, `ContainsKey`, `Remove`, `GetKeys()`, `GetValues()`.
  Case-insensitive string keys: `Map<String, int, SCaseI>`.
- `Set<T,TCompare>` - `Add` (replaces), `Contains`, `Remove`, `AddMany(coll)`, static `Union/Intersection/Difference`.
- Key types need hashing: ints, 64-bit ints, pointers and `String` work out of the box. For a custom key, give it `static uint32_t Hash(const T&)` plus `operator==`, or supply a comparer class with static `Hash` and `AreEqual` (see `SDefaultCompare` in `Core/Compare.h`; hash helpers in `Core/HashUtils.h`).

## Smart pointers

- `OwnedPtr<T>` - unique owner, move-only, `Detach()`. Assigning a raw pointer or another `OwnedPtr` deletes the previously held object.
- `SharedPtr<T>` - non-intrusive, **non-atomic** refcount (single-threaded only).
- `RefCountedPtr<T>` - for intrusive `AddRef()/Release()` objects (COM). Constructing/assigning from a raw `T*` **adopts** the reference (no AddRef). `&ptr` gives `T**` for out-params (asserts the pointer is null).

## Delegate (global namespace)

`Delegate<R(Args...)>` - heap-allocating `std::function`-like. `Delegate<R(Args...), N>` stores the callable inline in N bytes and **never allocates** (compile error if it doesn't fit) - use it on real-time/audio threads. `if (d) d(args);`.

## Threading - choosing the right tool

Real-time (audio) code must not allocate, lock or block. Choose by pattern:

| Need | Use |
|---|---|
| Plain mutual exclusion | `Mutex` + scoped `EnterMutex em(mx);` |
| Reader/writer lock | `SlimLock` + `EnterSlimLock l(lock, exclusive);` |
| Tiny critical section, light contention | `SpinLock` + `EnterSpinLock` |
| Atomic int/bool/ptr/float + futex wait | `Atomic<T>` (`Get/Set/Inc/Dec/Add/FetchAdd/CompareExchange/TrySet/Wait/WakeOne/WakeAll`) |
| Counting signal between threads | `Semaphore` (OS) or `AtomicSemaphore` (futex, `SpinWait`, `Stop`) |
| 1 producer -> 1 consumer FIFO | `SpscQueue<T>(capacity)` - `TryWrite/Read/Peek` |
| N producers -> N consumers, bounded | `MpmcQueue<T>(capacity)` - `TryWrite/MustWrite/Read` |
| Intrusive unbounded LIFO / FIFO (T has `T* next`) | `MpmcStack<T>` / `MpmcLinkedQueue<T>` (`Write`, `ReadAll` returns a chain) |
| Publish a small struct, readers never block writer | `AtomicStruct<T>` (seqlock) |
| UI thread hands latest object to audio thread and gets old ones back | `AtomicPtrTransfer<T>` (`Send`/`Receive`/`Return`/`Reclaim`) |
| UI thread edits a list, audio thread reads a snapshot each cycle | `CowList<T>` (or `CowListWops<T>` to also get inserted/deleted items) |
| Per-thread object | `ThreadLocal<T>` (`Get()` auto-creates) |
| Thread | subclass `Thread`, override `virtual void ThreadProc()`, call `Start()`, `Join()` |

Rules for the single-writer/single-reader types (`SpscQueue`, `AtomicPtrTransfer`, `CowList`, `CowListWops`): exactly one thread on each side; never call reader methods from the writer thread or vice versa. CowList snapshots (`GetSnapshot()`) are valid only until the next `GetSnapshot()`; batch multi-step edits with `StartUpdate()/EndUpdate()` so the reader never sees a half-applied change; `T` must be trivially copyable (PODs / raw pointers).

"Likely" methods (`GetLikelyCount`, `IsLikelyEmpty`) are approximate under concurrency - don't use them for correctness.

`kWaitForever` is the default timeout; timeouts are `uint32_t` milliseconds and wait functions return `false` on timeout.

## Streams, files, paths

```cpp
FileStream fs;
RIFE(fs.Open(path, "rb"));           // fopen-style modes: r w a, +, b
uint32_t magic; RIFE(fs.ReadValue(magic));
MemoryStream ms; ms.Create();        // growable in-memory stream
RIFE(Stream::Copy(ms, fs));
```
`Read(pv, cb)` without `pcb` returns non-zero on a short read (EOF); pass `&cb` to accept partial reads. `WriteLenString/ReadLenString` = uint32 length-prefixed UTF-8.

`Path::Join(a,b)`, `Combine(base,rel)` (canonicalizes), `GetDirectoryName`, `GetFileName`, `GetFileNameWithoutExtension`, `GetExtension`, `ChangeExtension`, `Canonicalize`, `GetFullPath`, `IsFullyQualified`, `DoesMatchPattern(name, "*.txt;*.md")`. All return `String`.
`FileSystem::Exists/Rename/Unlink/MakeDirectory/Stat/GetTempDirectory` (int error returns).

## JSON

Write-only: build `JSONValue` trees (`JSONObject`/`JSONArray` are ref-counted `Map`/`List`), then `JSON::Stringify(value, indentSize)`. **There is no parser** (`Parse` is commented out).

## Unit tests (`UnitTesting.h`)

```cpp
#include "../SimpleLib/UnitTesting.h"
#include "../SimpleLib/Core.h"
using namespace SimpleLib;            // required - the macros use unqualified names

Fact("List Add increases count")
{
    List<int> l;
    l.Add(1);
    Assert(l.GetCount() == 1);
}

Initialize()   { /* runs once before all facts */ }
Deinitialize() { /* runs once after */ }

// main.cpp
int main() { SimpleLib::TestRunner::Run(); return 0; }
```
Facts self-register at static init, one per line (the name is built from `__LINE__`, so don't put two on one line). `Assert` throws `AssertionFailed`, so build with exceptions on (`/EHsc`).

## Other utilities

`Nullable<T>` (`HasValue/GetValue/Clear`), `AutoRestore<T> ar(var, tempValue)` (restores on scope exit), `Bit::Count/Set/Clear/Scan/Find` (unsigned only), `Endian::ToBig/FromLittle/ByteSwap`, `cc4("RIFF")`/`cc8` FourCC constants, `Crc32::Calculate(p, cb)`, `Guid` (`Parse`, `ToString`), `StringPool<char>` (arena for many temporary C strings), `Buffer<T>` (raw POD scratch buffer, include `Core/Buffer.h` directly - not in Core.h), `Parse<char>` (pointer-advancing tokenizer helpers), `DynType`/`Dynamic<Self,Base>` (lightweight RTTI and factory-by-name), `Utils::Swap`.

Geo: `Vector<T>` / `Rectangle<T>` (typedefs `VectorD`, `RectangleI`, `RectangleF`...), `Affine<T>` 2D transform (`Translate/Rotate/Scale/Inverse`, static `MakeXxx`), `Matrix4<T>`, `ColorF` (`FromARGB`, `Interpolate`, `Blend`), `Math::Min/Max/Constrain/DegreesToRadian`.

## Avoid

- Anything under `.vscode/deprecated_todo/` (old HighWaterHeap, NodeClustering).
- Mixing STL containers into SimpleLib-based code when a SimpleLib equivalent exists.
