#pragma once

#include "string.h"
#include "sharedptr.h"
#include "list.h"
#include "dictionary.h"

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

    class JsonArray;
    class JsonObject;

    class JsonValue
    {
    public:
        virtual JsonKind GetKind() = 0;
        virtual void* GetNull() { assert(false); return nullptr; }
        virtual double GetNumber() { assert(false); return 0.0; }
        virtual String<char> GetString() { assert(false); return ""; }
        virtual bool GetBool() { assert(false); return false; }
        virtual JsonArray* GetArray() { assert(false); return nullptr; }
        virtual JsonObject* GetObject() { assert(false); return nullptr; }

        /*
        static JsonValue* FromNull() { return new JsonNull(); }
        static JsonValue* From(double value) { return new JsonNumber(value); }
        static JsonValue* From(const char* value) { return value == nullptr ? (JsonValue*)new JsonNull() : (JsonValue*)new JsonString(value); }
        static JsonValue* From(const String<char> value) { return value.IsNull() ? (JsonValue*)new JsonNull() : (JsonValue*)new JsonString(value); }
        static JsonValue* From(bool value) { return new JsonBoolean(value); }
        */
    };

    class JsonNull : public JsonValue
    {
        virtual JsonKind GetKind() override { return JsonKind::Null; }
        virtual void* GetNull() override { return nullptr; }
    };

    class JsonNumber : public JsonValue
    {
    public:
        JsonNumber(double value) { m_dblValue = value; }
        virtual JsonKind GetKind() override { return JsonKind::Number; }
        virtual double GetNumber() override { return m_dblValue; }
        double m_dblValue;
    };

    class JsonString : public JsonValue
    {
    public:
        JsonString(const char* value) { m_strValue = value; }
        JsonString(const String<char>& value) { m_strValue = value; }
        virtual JsonKind GetKind() override { return JsonKind::String; }
        virtual String<char> GetString() { return m_strValue; }
        String<char> m_strValue;
    };

    class JsonBoolean : public JsonValue
    {
    public:
        JsonBoolean(bool value) { m_bValue = value; }
        virtual JsonKind GetKind() override { return JsonKind::Boolean; }
        virtual bool GetBool() { return m_bValue; }
        bool m_bValue;
    };

    class JsonArray : 
        public JsonValue,
        public List<SharedPtr<JsonValue>>
    {
    public:
        virtual JsonKind GetKind() override { return JsonKind::Array; }
        virtual JsonArray* GetArray() override { return this; }
    };
    
    class JsonObject : 
        public JsonValue,
        public Dictionary<String<char>, SharedPtr<JsonValue>>
    {
    public:
        virtual JsonKind GetKind() override { return JsonKind::Object; }
        virtual JsonObject* GetObject() override { return this; }
    };


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
                        if (ch <= 0x1f || ch > 0x7f)
                            buf.Format("\\u%4X", ch);
                        else
                            buf.Append(ch);
                        break;
                }
            }
            buf.Append("\"");
        }

        static void Stringify(StringBuilder<char>& buf, JsonValue* value, int indentSize, int indent)
        {
            switch (value->GetKind())
            {
                case JsonKind::Null:
                    buf.Append("null");
                    break;

                case JsonKind::Boolean:
                    buf.Append(value->GetBool() ? "true" : "false");
                    break;

                case JsonKind::Number:
                    buf.Format("%.17g", value->GetNumber());
                    break;

                case JsonKind::String:
                    EscapeString(buf, value->GetString());
                    break;

                case JsonKind::Array:
                {
                    JsonArray* arr = value->GetArray();
                    buf.Append('[');
                    if (indentSize)
                    {
                        buf.Append("\n");
                        indent += indentSize;
                        for (int i=0; i<arr->GetCount(); i++)
                        {
                            buf.Append(' ', indent);
                            Stringify(buf, arr->GetAt(i), indentSize, indent);
                            if (i != arr->GetCount() - 1)
                                buf.Append(',');
                            buf.Append("\n");
                        }
                        indent -= indentSize;
                        buf.Append(indent);
                    }
                    else
                    {
                        for (int i=0; i<arr->GetCount(); i++)
                        {
                            Stringify(buf, arr->GetAt(i), indentSize, indent);
                            if (i != arr->GetCount() - 1)
                                buf.Append(',');
                        }
                    }
                    buf.Append(']');
                    break;
                }

                case JsonKind::Object:
                {
                    JsonObject* obj = value->GetObject();
                    buf.Append('{');
                    if (indentSize)
                    {
                        buf.Append("\n");
                        indent += indentSize;
                        for (int i=0; i<obj->GetCount(); i++)
                        {
                            buf.Append(' ', indent);
                            EscapeString(buf, obj->GetAt(i).Key);
                            buf.Append(": ");
                            Stringify(buf, obj->GetAt(i).Value, indentSize, indent);
                            if (i != obj->GetCount() - 1)
                                buf.Append(',');
                            buf.Append("\n");
                        }
                        indent -= indentSize;
                        buf.Append(indent);
                    }
                    else
                    {
                        for (int i=0; i<obj->GetCount(); i++)
                        {
                            EscapeString(buf, obj->GetAt(i).Key);
                            buf.Append(":");
                            Stringify(buf, obj->GetAt(i).Value, indentSize, indent);
                            if (i != obj->GetCount() - 1)
                                buf.Append(',');
                        }
                    }
                    buf.Append('}');
                    break;
                }
            }
        }

        static String<char> Stringify(JsonValue* value, int indentSize)
        {
            StringBuilder<char> buf;
            Stringify(buf, value, indentSize, 0);
            return buf;
        }

        static SharedPtr<JsonValue> Parse(const char* psz)
        {
        }
    };
}