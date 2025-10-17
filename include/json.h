#pragma once

#include "string.h"
#include "sharedptr.h"
#include "list.h"
#include "dictionary.h"

#ifdef _SIMPLELIB_USE_RYU
#include <ryu.h>
#endif

namespace SimpleLib
{
    enum class JsonKind
    {
        Null,
        Number,
        String,
        Boolean,
        Object,
        Array,
    };

    template <typename T>
    class RefCounted : public T
    {
    public:
        RefCounted()
        {
            m_iRef = 0;
        }
        virtual ~RefCounted()
        {
        }

        void AddRef()
        {
            m_iRef++;
        }
        void Release()
        {
            m_iRef--;
            if (m_iRef == 0)
                delete this;
        }

        int m_iRef;
    };

    class JSONArray;
    class JSONObject;

    struct JSONValue
    {
        // Move constructor
		JSONValue(JSONValue&& other)
		{
            kind = other.kind;
            memcpy(&value, &other.value, sizeof(value));
            other.kind = JsonKind::Null;
		}

        // Copy constructor
        JSONValue(const JSONValue& other)
        {
            Assign(other);
        }

        JSONValue()
        {
            kind = JsonKind::Null;
        }

        JSONValue(double val)
        {
            kind = JsonKind::Number;
            value.number = val;
        }
        JSONValue(int val)
        {
            kind = JsonKind::Number;
            value.number = val;
        }
        JSONValue(bool val)
        {
            kind = JsonKind::Boolean;
            value.boolean = val;
        }
        JSONValue(const char* psz)
        {
            SetString(psz);
        }
        JSONValue(JSONArray* val);
        JSONValue(JSONObject* val);
        ~JSONValue()
        {
            Reset();
        }

        void Reset();
        void Assign(const JSONValue& val);

        JSONValue& operator =(const JSONValue& val)
        {
            Reset();
            Assign(val);
            return *this;
        }

        double AsNumber() const
        {
            assert(kind == JsonKind::Number);
            return value.number;
        }
        bool AsBool() const
        {
            assert(kind == JsonKind::Boolean);
            return value.boolean;
        }
        const char* AsString() const
        {
            assert(kind == JsonKind::String);
            return value.string;
        }
        JSONArray& AsArray() const;
        JSONObject& AsObject() const;

        JsonKind kind;
        union _value
        {
            double number;
            bool boolean;
            const char* string;
            JSONArray* array;
            JSONObject* object;
        } value;

        void SetString(const char* psz)
        {
            if (psz == nullptr)
            {
                kind = JsonKind::Null;
            }
            else
            {
                size_t len = strlen(psz) + 1;
                char* mem = (char*)malloc(len);
                memcpy(mem, psz, len);
                kind = JsonKind::String;
                value.string = mem;
            }
        }

    };

    class JSONArray : public RefCounted<List<JSONValue>>
    {
    };
    class JSONObject : public RefCounted<Dictionary<String<char>, JSONValue>>
    {
    };

    void JSONValue::Assign(const JSONValue& other)
    {
        kind = other.kind;
        switch (kind)
        {
            case JsonKind::Null:
                break;
            case JsonKind::Number:
                value.number = other.value.number;
                break;
            case JsonKind::Boolean:
                value.boolean = other.value.boolean;
                break;
            case JsonKind::String:
                SetString(other.value.string);
                break;
            case JsonKind::Array:
                value.array = other.value.array;
                value.array->AddRef();
                break;
            case JsonKind::Object:
                value.object = other.value.object;
                value.object->AddRef();
                break;
        }
    }


    inline JSONValue::JSONValue(JSONArray* val)
    {
        kind = JsonKind::Array;
        value.array = val;
        value.array->AddRef();
    }
    inline JSONValue::JSONValue(JSONObject* val)
    {
        kind = JsonKind::Object;
        value.object = val;
        value.object->AddRef();
    }
    inline void JSONValue::Reset()
    {
        switch (kind)
        {
            case JsonKind::String:
                free((char*)value.string);
                break;
            case JsonKind::Array:
                value.array->Release();
                break;
            case JsonKind::Object:
                value.object->Release();
                break;
            default:
                break;
        }
        kind = JsonKind::Null;
    }


    inline JSONArray& JSONValue::AsArray() const
    {
        assert(kind == JsonKind::Array);
        return *value.array;
    }
    inline JSONObject& JSONValue::AsObject() const
    {
        assert(kind == JsonKind::Object);
        return *value.object;
    }

    struct JSON
    {
        static void EscapeString(StringBuilder<char>& buf, String<char> value)
        {
            buf.Append("\"");
            for (int i=0; i<value.GetLength(); i++)
            {
                char ch = value[i];
                switch (ch)
                {
                    case '\\': buf.Append("\\\\"); break;
                    case '/': buf.Append("\\/"); break;
                    case '\b': buf.Append("\\b"); break;
                    case '\f': buf.Append("\\f"); break;
                    case '\n': buf.Append("\\n"); break;
                    case '\r': buf.Append("\\r"); break;
                    case '\t': buf.Append("\\t"); break;
                    default:
                        if (ch <= 0x1f)
                            buf.Format("\\u%4X", ch);
                        else
                            buf.Append(ch);
                        break;
                }
            }
            buf.Append("\"");
        }

        static void Stringify(StringBuilder<char>& buf, const JSONValue& value, int indentSize, int indent)
        {
            switch (value.kind)
            {
                case JsonKind::Null:
                    buf.Append("null");
                    break;

                case JsonKind::Boolean:
                    buf.Append(value.AsBool() ? "true" : "false");
                    break;

                case JsonKind::Number:
#ifdef _SIMPLELIB_USE_RYU
                    char szTemp[128];
                    d2s_buffered(value.AsNumber(), szTemp);
                    remove_exponent_if_shorter(szTemp);
                    buf.Append(szTemp);
#else
                    buf.Format("%.17g", value.AsNumber());
#endif
                    break;

                case JsonKind::String:
                    EscapeString(buf, value.AsString());
                    break;

                case JsonKind::Array:
                {
                    JSONArray& arr = value.AsArray();
                    buf.Append('[');
                    if (indentSize)
                    {
                        buf.Append("\n");
                        indent += indentSize;
                        for (int i=0; i<arr.GetCount(); i++)
                        {
                            buf.Append(' ', indent);
                            Stringify(buf, arr.GetAt(i), indentSize, indent);
                            if (i != arr.GetCount() - 1)
                                buf.Append(',');
                            buf.Append("\n");
                        }
                        indent -= indentSize;
                        buf.Append(indent);
                    }
                    else
                    {
                        for (int i=0; i<arr.GetCount(); i++)
                        {
                            Stringify(buf, arr.GetAt(i), indentSize, indent);
                            if (i != arr.GetCount() - 1)
                                buf.Append(',');
                        }
                    }
                    buf.Append(']');
                    break;
                }

                case JsonKind::Object:
                {
                    JSONObject& obj = value.AsObject();
                    buf.Append('{');
                    if (indentSize)
                    {
                        buf.Append("\n");
                        indent += indentSize;
                        for (int i=0; i<obj.GetCount(); i++)
                        {
                            buf.Append(' ', indent);
                            EscapeString(buf, obj.GetAt(i).Key);
                            buf.Append(": ");
                            Stringify(buf, obj.GetAt(i).Value, indentSize, indent);
                            if (i != obj.GetCount() - 1)
                                buf.Append(',');
                            buf.Append("\n");
                        }
                        indent -= indentSize;
                        buf.Append(indent);
                    }
                    else
                    {
                        for (int i=0; i<obj.GetCount(); i++)
                        {
                            EscapeString(buf, obj.GetAt(i).Key);
                            buf.Append(":");
                            Stringify(buf, obj.GetAt(i).Value, indentSize, indent);
                            if (i != obj.GetCount() - 1)
                                buf.Append(',');
                        }
                    }
                    buf.Append('}');
                    break;
                }
            }
        }

        static String<char> Stringify(const JSONValue& value, int indentSize)
        {
            StringBuilder<char> buf;
            Stringify(buf, value, indentSize, 0);
            return buf;
        }

        /*
        static SharedPtr<JsonValue> Parse(const char* psz)
        {
        }
        */

        static void remove_exponent_if_shorter(char* psz)
        {
            // Scan the string looking for dot position and exponent position
            char* p = psz;
            char* pszDot = nullptr;
            char* pszExp = nullptr;
            while (*p)
            {
                if (*p == '.')
                    pszDot = p;
                if (*p == 'e' || *p == 'E')
                    pszExp = p;
                p++;
            }
            if (!pszExp)        // Not exponent format, quit
                return;

            // Get the exponent
            int e = atoi(pszExp+1);

            // Conversion depends on dp position and e
            if (e == 0)
            {
                // Just truncate and be done
                *pszExp = '\0';
                return;
            }

            // Work out where to move the decimal point to
            if (!pszDot)
                pszDot = pszExp;
            const char* pszNewDotPos = pszDot + e;

            if (e > 0 && e <= 20)
            {

                // If it's past the end of the current string, then there's no point
                if (pszNewDotPos > p)
                    return;

                // Move it, and add trailing zeros
                for (int i=0; i<e; i++)
                {
                    *pszDot = pszDot + 1 < pszExp ? pszDot[1] : '0';
                    pszDot++;
                }
                if (pszDot < pszExp)
                {
                    *pszDot = '.';
                    *pszExp = '\0';
                }
                else
                {
                    *pszDot = '\0';
                }
                return;
            }
            else if (e >= -6 && e < 0)
            {
                if (pszNewDotPos <= psz)
                {
                    // Make room
                    int shift = (int)(psz - pszNewDotPos + 1);
                    memmove(psz + shift, psz, p - psz + 1);
                    memset(psz, '0', shift);
                    pszExp += shift;
                    pszDot += shift;
                }

                for (int i=0; i < -e; i++)
                {
                    *pszDot = pszDot[-1];
                    pszDot--;
                }

                *pszDot = '.';
                *pszExp = '\0';
                return;
            }
        }
    };
}