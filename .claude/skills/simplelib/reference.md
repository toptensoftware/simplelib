# SimpleLib API reference

Signatures are abridged. `TArg` is the element's argument type (normally `T`; `T*` for `OwnedPtr<T>`). All in `namespace SimpleLib` except `Delegate`.

## Core/String.h - `StringCore<T>` (`String`, `WString`, `String32`)

Construct: `()` (null), `(const T*)`, `(const T*, int len)`, `(const StringBuilder<T>&)`. Copy is a ref-count bump.

| Method | Notes |
|---|---|
| `const T* sz() const` / `operator const T*` | `nullptr` when null |
| `int GetLength() const`, `bool IsNull()`, `bool IsEmpty()`, `operator!` (null or empty) | |
| `static bool IsNullOrEmpty(const T*)` | |
| `void Assign(const T*, int len=-1)`, `void Clear()` | |
| `ToUpper()`, `ToLower()` | non-const |
| `SubString(int start, int len=-1)` | non-const; negative start counts from end |
| `LTrim()`, `RTrim()`, `Trim()` | const |
| `IndexOf<S=SCase>(T ch \| const T* s, int start=0)` | -1 if not found |
| `LastIndexOf<S>(const T*, int start=-1)`, `IndexOfAny<S>(chars, start)`, `LastIndexOfAny<S>(chars, start)` | |
| `Replace<S>(find, replace, int max=-1, int start=0)` | non-const, returns new string |
| `IsEqualTo<S>(const T*)`, `StartsWith<S>(const T*)`, `EndsWith<S>(const T*)` | `S` = `SCase` or `SCaseI` |
| `int Split<S>(const T* separatorChars, bool includeEmpty, List<StringCore>& out) const` | separators are a set of single chars |
| `static Join(List<StringCore>&, T sep \| const T* sep)` | |
| `static Format(const T* fmt, ...)`, `FormatV` | see formatting below |
| `bool CopyToBuffer(T* buf, int bufLenChars)` | false if too small |
| `T* AllocCopy()` | caller `free()`s |
| `==, !=, <, >, <=, >=`, `operator+(String)`, `operator+=(const T* \| String)` | case-sensitive |
| `static uint32_t Hash(const StringCore&)` | lets String be a Map/Set key |

**Format specifiers** (`Formatting::FormatV`, identical on all platforms): `%[-][+][ ][#][0][width|*][.prec|*][l|ll|z]type`, types `c s d i u x X o p P f g`, `%%`. `%s` takes a `const T*` of the same char type as the format (`(null)` printed for nullptr). No `%S`, `%e`, `%ls`.

`SCase` / `SCaseI` (Core/StringSemantics.h): static `Compare(a,b[,len])`, `AreEqual(a,b)`, `Hash(a)` for char/wchar_t (SCase also char16_t/char32_t). Used as the `S` template arg above and as a Map/Set comparer.

## Core/StringBuilder.h - `StringBuilder<T>`

128-char inline buffer, then heap. `Append(T)`, `Append(T, count)`, `Append(const T*)`, `Append(const T*, len)`, `+=`, `T* Reserve(len)` (returns writable space at end), `T* GetBuffer(len)` (clears first), `SyncLength()` (after writing via C API into GetBuffer), `Format(fmt, ...)`, `FormatV`, `ReplaceAppend<S>(input, find, replace, max)`, `GetLength()`, `Truncate(n)`, `Clear()` (keeps memory), `Reset()` (frees), `sz()` / `Finish()` (null-terminates, doesn't reset), `ToString()`, `T* Detach()` (caller frees). Implements `IStringWriter<T>`.

## Core/Encoding.h

`template <TTo, TFrom> StringCore<TTo> Encode(const TFrom* in)` - UTF-8 <-> UTF-16/wchar_t <-> UTF-32. Lower level: `Encoding<TFrom,TTo>::Process(ch, IStringWriter<TTo>&)`.

## Core/Parse.h - `Parse<T>` (static, pointer-advancing, `const T*& p`)

`IsWhiteSpace`, `SkipWhiteSpace(p)`, `IsEOL`, `SkipEOL(p)`, `SkipToEOL(p)`, `SkipToNextLine(p)`, `IsOneOf(ch, chars)`, `IsDigit`, `ParseIdentifier(p, String& out, otherLead=nullptr, other=nullptr)`, `ParseInteger<TInt>(p, TInt& val)`, `ParseHexDigit(ch, uint8_t&)`, `ParseHexInteger<TUInt>(p, val)`. All return bool; on failure `p` is left unchanged.

## Core/StringPool.h - `StringPool<T>(bucketSize=4096)`

Arena: `T* Alloc(const T*)`, `Alloc(psz, len)`, `Reserve(len)`, `FreeAll()` (keep memory), `Reset()` (free memory). Individual strings can't be freed.

## Core/List.h - `List<T, TAllocator=TMalloc>`

Move-only. Grows to max(16, 2x).

- Size: `GetCount()`, `IsEmpty()`, `SetCapacity(n)`, `SetCount(n, fillVal)`, `FreeExtra()`, `TStorage* GetBuffer()`
- Access: `GetAt(i)`, `operator[](i)` (both return `TArg` by value, read-only), `GetRefAt(i)` (reference - use to mutate in place), `Head()/Tail()`, `TryHead(out)/TryTail(out)`
- Add/insert: `int Add(val)`, `InsertAt(i, val)`, `AddRange(const List&)`, `AddRange(List&&)`, `InsertRangeAt(i, list)`, `AddMany(coll)` / `InsertManyAt(i, coll)` (any collection with `Iterate()`), `ReplaceAt(i, val)`
- Remove: `RemoveAt(i)`, `RemoveAt(i, count)`, `int Remove(val)` (returns index or -1), `Clear()`, `TArg DetachAt(i)` / `DetachAll()` (remove without destroying owned objects)
- Reorder: `Swap(a, b)`, `Move(from, to)`, `Sort<TCompare=SDefaultCompare>()`, `Sort(int(*)(TArg,TArg))`, `Sort(int(*)(TArg,TArg,void*), void* user)` - comparers are function pointers (captureless lambdas OK)
- Search: `IndexOf<TCompare>(val, startAfter=-1)`, `Contains(val)`, `BinarySearch(int& index, TKey key, cmp)` where `cmp(elem, key)` returns <0/0/>0 (either `int(*)(TArg,TKey,void*)` + user, or any callable `(TArg,TKey)`); on miss `index` is the insertion point
- Stack/queue: `Push`, `Pop`, `TryPop(out)`, `Enqueue`, `Dequeue`, `TryDequeue(out)` (Dequeue is O(n))
- Functional: `List Filter(Delegate<bool(TArg)>)`, `List<R> Map<R>(Delegate<R(TArg)>)`
- Iterate: `Iterate()`, `IterateReverse()` -> `Iter` with `Next()`, `Get()`

## Core/Map.h - `Map<TKey, TValue, TKeyCompare=SDefaultCompare, TAllocator=TMalloc>`

Move-only hash map. Insertion-ordered iteration until removals happen.
`GetCount()`, `IsEmpty()`, `Add(k, v)` (ignored if key exists), `Set(k, v)` (upsert), `bool Remove(k)`, `Clear()`, `Get(k)` / `operator[](k)` (assert if missing, read-only), `Get(k, default)`, `bool TryGetValue(k, TValueArg& out)`, `ContainsKey(k)`, `List<TKeyArg> GetKeys()`, `List<TValueArg> GetValues()`, `Iterate()`/`IterateReverse()` -> `Iter` with `Next()`, `GetKey()`, `GetValue()`.

To change a value: `map.Set(k, newValue)` (there's no reference accessor).

## Core/Set.h - `Set<T, TCompare=SDefaultCompare, TAllocator=TMalloc>`

Move-only. `Add(v)`, `AddMany(coll)`, `bool Remove(v)`, `Contains(v)`, `Clear()`, `GetCount()`, `IsEmpty()`, `Iterate()` -> `Get()`, static `Union(a,b)`, `Intersection(a,b)`, `Difference(a,b)` (a minus b).

## Core/Compare.h - comparers

`SDefaultCompare`: `Compare(a,b)` (uses `<`/`>`; Strings via SCase), `AreEqual(a,b)` (`==`), `Hash(a)` for int32/uint32/int64/uint64/pointers, or `T::Hash(const T&)` if the type defines it (else static_assert). Custom comparer = class with static `Compare`, `AreEqual`, `Hash`. Hash helpers: `hash32`, `hash32_64`, `hash_buf(p, len, seed)`, `hash_str` in Core/HashUtils.h.

## Smart pointers

- `OwnedPtr<T>(T* = nullptr)`: `Detach()`, `operator T*`, `->`, `*`, `operator bool`. Move-only. Deletes on destruction.
- `SharedPtr<T>(T* = nullptr)`: copyable, non-atomic count, `==`/`!=`, `operator T*` asserts when null.
- `RefCountedPtr<T>(T* = nullptr)`: adopts on raw construct/assign, AddRef on copy, Release on destroy, `Detach()`, `T** operator&()`.

## Core/Delegate.h - `Delegate<R(Args...), StorageSize=0>` (global namespace)

Construct from any callable or `nullptr`. `operator()(args)`, `valid()`, `explicit operator bool`. Copyable and movable. `StorageSize=0` heap-allocates; `>0` is inline-only, never allocates, static_asserts on size.

## Misc Core

- `Nullable<T>`: `()`, `(const T&)`, `HasValue()`, `GetValue()` (asserts), `= value`, `Clear()`.
- `AutoRestore<T>(T& var)` / `(T& var, const T& newValue)`; `OldValue()`. Non-copyable.
- `Bit::Count(n)`, `Set(n, bit[, bool])`, `Clear(n, bit)`, `Scan(mask)` (first set bit), `Find(mask, index)` (index'th set bit) - unsigned types only; -1 if none.
- `Endian::GetKind()`, `ByteSwap(x)` (16/32/64 int, float, double), `ToBig`, `ToLittle`, `FromBig`, `FromLittle`.
- `cc4("abcd")` -> `uint32_t`, `cc8("abcdefgh")` -> `uint64_t` (constexpr, memory byte order matches the string).
- `Crc32`: `Update(byte)`, `Update(p, cb)`, `Finish()`, `Reset()`, static `Calculate(p, cb)`.
- `Guid` (`Data1..Data4`): `==`, `ToString()` (dashed), `ToStringCompact()`, static `Parse(str, out)` (accepts braces/dashes), `ParseCompact(str, out)`.
- `DynType` / `Dynamic<TSelf, TBase>` / `DynamicCreatable<TSelf, TBase, ID>`: custom RTTI. `obj->QueryAs<T>()` (nullptr if wrong type), `obj->As<T>()` (asserts), `DynType::GetTypeFromID/GetTypeFromName`, `type->CreateInstance()`. Override `static const char* GetTypeName()` to register a name.
- `Utils::Swap(a, b)`.
- `Buffer<T>(initialSize=0)` (Core/Buffer.h, not included by Core.h): uninitialized POD buffer, non-copyable. `operator T*`, `SetCapacity(n)` (resize, keeps contents; 0 frees), `EnsureSize(n)` (grow only), `GetCapacity()`.

## Threading (Threading.h)

Constants (Threading/Types.h): `kWaitForever`, `kCacheLineSize = 64`, `enum class ThreadPriority { BelowNormal, Normal, AboveNormal, RealTime }`.

- `Atomic<T>` (T up to pointer size; narrow types stored in uint32): `Atomic(T initial)`, `Get()`, `Set(v)` (returns previous), `CompareExchange(val, compare)` (returns previous), `TrySet(val, compare)` (bool), `Inc()`, `Dec()`, `Add(d)` (return new value), `FetchAdd(d)` (returns old), `Wait(expected, timeout)` (futex: block while value == expected), `WakeOne()`, `WakeAll()`. `Atomic<float>` has no arithmetic. Not copyable.
- `AtomicTaggedPtr<T>`: 128-bit pointer+tag CAS for ABA-safe stacks: `Get()`, `TrySet(Value, Value)`, `Value{ptr, tag}`.
- `AtomicStruct<T>(initial)`: `Get(T& out)`, `Set(const T&)` - seqlock, single writer.
- `AtomicSemaphore(initial)`: `Release(n=1)`, `Wait(timeout)`, `SpinWait(spinCount, timeout, onIdle=nullptr, user=nullptr)`, `Stop()` (wakes all waiters, they return false), `Reset()` (only after waiters have exited).
- `Semaphore(initial)`: `Wait(timeout)`, `Release(count=1)`, `Init(initial)`.
- `Mutex`: `Enter()`, `TryEnter()`, `Leave()`, `IsHeld()`. RAII: `EnterMutex em(mx);` (or default-construct then `Enter(mx)`, `Leave()`).
- `SlimLock` (SRW): `EnterExclusive/TryEnterExclusive/LeaveExclusive`, `EnterShared/TryEnterShared/LeaveShared`, `Enter(bool exclusive)`, `Leave(bool exclusive)`. RAII: `EnterSlimLock l(lock, exclusive);`, `TryEnter(lock, exclusive)`.
- `SpinLock` (busy-wait, cache-line aligned): `Enter()`, `TryEnter()`, `Leave()`. RAII: `EnterSpinLock`.
- `Thread(priority=Normal, description=nullptr)`: subclass and override `protected: virtual void ThreadProc()`. `Start()`, `Join(timeout)`, `SetPriority`, `SetDescription`, `GetId()`, static `GetCurrentId()`, `GetCurrentHandle()`, `Yield()`, `Sleep(ms)`. ThreadLocal objects are freed automatically when a SimpleLib Thread exits.
- `ThreadLocal<T, autoCreate=true>`: `T* Get()` (creates `new T()` on first use), `T* Set(T*)` (old value not deleted), `Free()`. Threads not created via `Thread` should call `ThreadLocalBase::FreeAll()` before exiting.
- `SpscQueue<T>(capacity)` - one writer, one reader: `TryWrite(v)`, `MustWrite(v)` (asserts if full; returns true if it was empty, useful to signal the reader), `Read(T& out)`, `Peek(T&)`, `Peek(offset, T&)`, `RemoveAll()`, `Reset(newCapacity=-1)`, `GetCapacity()`, `GetLikelyCount()`, `GetLikelyAt(i)`, `IsLikelyEmpty/Full()`.
- `MpmcQueue<T>(capacity)` - bounded, Vyukov algorithm, capacity >= 2: `TryWrite(v)`, `MustWrite(v)` (asserts if full), `Read(T& out)`, `RemoveAll()`, `Reset(capacity)`, `GetCapacity()`, `GetLikelyCount()`, `IsLikelyEmpty/Full()`.
- `MpmcStack<T>` - intrusive (`T* next`, must be nullptr when pushed), unbounded LIFO: `bool Push(T*)` (true if it was empty), `Push(first, last)` (chain), `Pop()`, `PopAll()`, `GetLikelyCount()`, `Reset()`. The container never allocates or frees nodes.
- `MpmcLinkedQueue<T>` - intrusive FIFO built on MpmcStack: `bool Write(T*)` (true if it was empty), `T* ReadAll()` (FIFO chain via `next`, last node's next is nullptr; clear `next` on each node before re-writing it), `GetLikelyCount()`, `Reset()`. No single-item read.
- `AtomicPtrTransfer<T, TRelease=TDelete<T>>` - one sender, one receiver:
  - sender: `Send(T*)` (disposes returned/unclaimed objects via TRelease), or `T* SendNoReclaim(T*)` (returns the unclaimed previous object) + `T* Reclaim()` in a loop.
  - receiver: `T* Receive()` (latest object or nullptr; the slot keeps only the newest), `Return(T*)` (hand back an object you received). Receiver never allocates or blocks.
- `CowList<T>` (T trivially copyable) - writer thread: `Add`, `InsertAt`, `RemoveAt`, `Move`, `Remove(v)`, `Find(v)`, `IndexOf(v)`, `Clear`, `Sort...`, `SetCapacity`, `GetCount`, `GetAt`, `StartUpdate()/EndUpdate()` (nestable batch), `Reset()`. Reader thread: `CowListSnapshot<T>& GetSnapshot()` -> `GetCount()`, `GetAt(i)`, `[i]`.
- `CowListWops<T>` - same as CowList, plus the snapshot has `GetInsertedCount()/GetInsertedItem(i)` and `GetDeletedCount()/GetDeletedItem(i)`: the set of items added/removed since the reader's previous snapshot. Insert followed by remove before a snapshot cancels out; moves aren't reported. T must support `==`.

## Stream (Stream.h)

`Stream` (abstract): `IsOpen()`, `int Read(pv, cb, size_t* pcb=nullptr)`, `int Write(pv, cb, size_t* pcb=nullptr)`, `int Seek(int64 off, origin=SEEK_SET)`, `int64 Tell()`, `int64 GetLength()`, `int Truncate()`, `int Flush()`, `IsEof()`, `WriteValue<T>(v)`, `ReadValue<T>(T&)`, `T ReadValue<T>()`, `Write(const char*)`, `WriteLenString(psz)`, `String ReadLenString()`, static `Copy(dest, src[, length])`. All ints are 0 on success. `RIFE(x)` macro returns early on non-zero.

- `FileStream`: `int Open(path, mode)` (`"r" "w" "a"` + optional `"+"`, `"b"` ignored; UTF-8 path), `Close()`.
- `MemoryStream`: `Create()` (empty, growable, owned), `Open(p, len, takeOwnership)` (wrap existing memory), `Init(p, len)` (copy), `OpenResource(hModule, name, type)` (Windows), `Close()`, `void* GetBuffer()`, `void* CloseAndDetach(size_t* pcb)` (caller frees), `MoveFrom(other)`, `IsEqual(other)`, `WriteTo(stream)`, `ReadFrom(stream)`.

## Path (Path.h) - `Path` = `PathCore<SPathSemanticsAuto>`

Also `PathCore<SPathSemanticsWindows>` / `PathCore<SPathSemanticsPosix>` to force a style. All static, all take `const char*`, return `String`:
`Join(a, b)` (simple join, one separator), `Combine(base, path)` (rooted `path` replaces base, result canonicalized), `GetDirectoryName(p)` (null if none; keeps root separator), `GetFileName(p)`, `GetFileNameWithoutExtension(p)`, `GetExtension(p)` (includes the dot), `const char* FindExtension(p)`, `ChangeExtension(p, ext)`, `Canonicalize(p)` (collapses `.`, `..`, duplicate separators lexically), `GetCurrentDirectory()`, `GetFullPath(p)`, `bool IsFullyQualified(p)`, `int GetPrefixLength(p)` (drive/UNC), `bool DoesMatchPattern(name, "*.a;*.b")` (case-insensitive on Windows).

## FileSystem (FileSystem.h)

Static: `bool Exists(path)`, `String GetTempDirectory()`, `int Rename(old, new)`, `int Unlink(path)`, `int Stat(path, Stat* out)` (`out->size`, `out->mtime`), `int MakeDirectory(path)`.

## Json (Json.h) - write-only

`JSONValue`: constructors from `double`, `int`, `bool`, `const char*`, `JSONArray*`, `JSONObject*`; `kind` (`JsonKind::Null/Number/StringCore/Boolean/Object/Array` - strings are `StringCore`), `AsNumber()`, `AsBool()`, `AsString()`, `AsArray()`, `AsObject()`. `JSONArray` is a ref-counted `List<JSONValue>`, `JSONObject` is a ref-counted `Map<String, JSONValue>`; create them with `new` and wrap them in a JSONValue straight away (that adds the reference).
```cpp
auto obj = new JSONObject();
JSONValue root(obj);
obj->Set("name", "x");
obj->Set("count", 3);
String text = JSON::Stringify(root, 2);   // indentSize; 0 = compact
```
Define `_SIMPLELIB_USE_RYU` (needs ryu) for shortest round-trip number formatting.

## Geo (Geo.h)

- `Vector<T>` (`X`, `Y`): `+ - * /`, unary `-`, `Magnitude()`, `Normal()`, `Rotate(angle)`, `IsInsideTriangle(A,B,C)`, static `Distance(a,b)`, `DotProduct(a,b,c)`, `CrossProduct(a,b,c)`. Typedefs `VectorI/VectorD/VectorF`.
- `Rectangle<T>(left, top, width, height)` (`Left`, `Top`, `Width`, `Height`): `Right()`, `Bottom()`, `setRight`, `setBottom`, `Center()`, `CenterX/Y()`, `TopLeft()`..., `Area()`, `IsNormal()`, `Offset`, `Inflate`, `Inset`, `Outset`, `Intersection`, `Union`, `Contains(x,y | Vector | Rectangle)`, static `FromCoords(x1,y1,x2,y2)`. Typedefs `RectangleI/RectangleD/RectangleF`.
- `Affine<T>` (`M11 M12 M21 M22 M31 M32`, default identity): `*`, chainable `Translate`, `Rotate(radians)`, `RotateAround`, `Scale`, `Skew`, `Inverse()`, `Determinant()`, static `MakeTranslate/MakeRotate/MakeScale/MakeSkew/MakeRotateAround`.
- `Matrix4<T>`: `Inverse`, `Transpose`, chainable `Translate/RotateX/Y/Z/Scale`, static `MakeTranslate/MakeRotateX/Y/Z/MakeScale/MakeOrthoProjection(l,t,r,b,near,far)`.
- `ColorF(r, g, b, a=1)` floats 0..1: `AsARGB/AsABGR/AsRGBA()`, static `FromARGB/FromABGR/FromRGBA(uint32)`, `Interpolate(a, b, pos)`, `Blend(back, fore)`.
- `Math`: `Min`, `Max`, `Normalize(a,b)` (swap so a<=b), `Constrain(v, min, max)` (works with inverted ranges), `PI`, `DegreesToRadian`.
