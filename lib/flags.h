#ifndef FLAGS_H
#define FLAGS_H

class DataSpec;

class Flag
{
public:
    static void parseFlags(int argc, const char* argv[]);

    Flag(const std::vector<std::string>& names, const std::string helptext);
    virtual ~Flag() {};

    const std::string& name() const { return _names[0]; }
    const std::vector<std::string> names() const { return _names; }
    const std::string& helptext() const { return _helptext; }

    virtual bool hasArgument() const = 0;
    virtual const std::string defaultValueAsString() const = 0;
    virtual void set(const std::string& value) = 0;

private:
    const std::vector<std::string> _names;
    const std::string _helptext;
};

class ActionFlag : Flag
{
public:
    ActionFlag(const std::vector<std::string>& names, const std::string helptext,
            std::function<void(void)> callback):
        Flag(names, helptext),
        _callback(callback)
    {}

    bool hasArgument() const { return false; }
    const std::string defaultValueAsString() const { return ""; }
    void set(const std::string& value) { _callback(); }

private:
    const std::function<void(void)> _callback;
};

class SettableFlag : public Flag
{
public:
    SettableFlag(const std::vector<std::string>& names, const std::string helptext):
        Flag(names, helptext)
    {}

    operator bool() const { return _value; }

    bool hasArgument() const { return false; }
    const std::string defaultValueAsString() const { return "false"; }
    void set(const std::string& value) { _value = true; }

private:
    bool _value = false;
};

template <typename T>
class ValueFlag : public Flag
{
public:
    ValueFlag(const std::vector<std::string>& names, const std::string helptext,
            const T defaultValue):
        Flag(names, helptext),
        defaultValue(defaultValue),
        value(defaultValue)
    {}

    operator T() const { return value; }

    bool hasArgument() const { return true; }

    T defaultValue;
    T value;
};

class StringFlag : public ValueFlag<std::string>
{
public:
    StringFlag(const std::vector<std::string>& names, const std::string helptext,
            const std::string defaultValue = ""):
        ValueFlag(names, helptext, defaultValue)
    {}

    const std::string defaultValueAsString() const { return defaultValue; }
    void set(const std::string& value) { this->value = value; }
};

class IntFlag : public ValueFlag<int>
{
public:
    IntFlag(const std::vector<std::string>& names, const std::string helptext,
            int defaultValue = 0):
        ValueFlag(names, helptext, defaultValue)
    {}

    const std::string defaultValueAsString() const { return std::to_string(defaultValue); }
    void set(const std::string& value) { this->value = std::stoi(value); }
};

class DoubleFlag : public ValueFlag<double>
{
public:
    DoubleFlag(const std::vector<std::string>& names, const std::string helptext,
            double defaultValue = 1.0):
        ValueFlag(names, helptext, defaultValue)
    {}

    const std::string defaultValueAsString() const { return std::to_string(defaultValue); }
    void set(const std::string& value) { this->value = std::stod(value); }
};

class BoolFlag : public ValueFlag<double>
{
public:
    BoolFlag(const std::vector<std::string>& names, const std::string helptext,
            bool defaultValue = false):
        ValueFlag(names, helptext, defaultValue)
    {}

    const std::string defaultValueAsString() const { return defaultValue ? "true" : "false"; }
    void set(const std::string& value);
};

#endif
