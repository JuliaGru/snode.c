#ifndef MULTIPLEXER_H
#define MULTIPLEXER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/select.h> // for fd_set

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedExceptions.h"
#include "ManagedReader.h"
#include "ManagedServer.h"
#include "ManagedTimer.h"
#include "ManagedWriter.h"


class Multiplexer {
private:
    Multiplexer();

public:
    static Multiplexer& instance() {
        return multiplexer;
    }

    ManagedReader& getManagedReader() {
        return managedReader;
    }

    ManagedServer& getManagedServer() {
        return managedServer;
    }

    ManagedWriter& getManagedWriter() {
        return managedWriter;
    }

    ManagedExceptions& getManagedExceptions() {
        return managedExceptions;
    }

    ManagedTimer& getManagedTimer() {
        return managedTimer;
    }

    static void init(int argc, char* argv[]);
    static void start();
    static void stop();

private:
    static void stoponsig(int sig);

    inline void tick();

    static Multiplexer multiplexer;

    fd_set readfds{0};
    fd_set writefds{0};
    fd_set exceptfds{0};

    ManagedReader managedReader;
    ManagedServer managedServer;
    ManagedWriter managedWriter;
    ManagedExceptions managedExceptions;
    ManagedTimer managedTimer;

    static bool running;
    static bool stopped;
};


#endif // MULTIPLEXER_H
