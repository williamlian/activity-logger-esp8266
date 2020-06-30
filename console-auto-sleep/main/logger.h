#ifndef ACTIVITY_LOGGER_H
#define ACTIVITY_LOGGER_H
class ActivityLogger {
    public:
    ActivityLogger(const char* module) {
        this->module = module;
    }
    void log(const char* fmt, ...);
    private:
    const char* module;
};
#endif
