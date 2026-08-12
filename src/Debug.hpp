#ifndef A2E_DEBUG_HPP
#define A2E_DEBUG_HPP

// Plain-file debug logging, alongside DBPrintf (Console.app on Mac) —
// no dependency on any OS logging subsystem. Watch it live with
// `tail -f /tmp/a2e_debug.log`.
void A2E_LogToFile (const char* format, ...);

// Logs to both channels — whichever one actually works gives us
// signal. Same call shape as DBPrintf (printf-style format string).
#define A2E_TRACE(...) do { DBPrintf (__VA_ARGS__); A2E_LogToFile (__VA_ARGS__); } while (0)

#endif
