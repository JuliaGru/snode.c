#ifndef MANAGER_H
#define MANAGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <list>
#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"
#include "ManagedDescriptor.h"

class Multiplexer;

template <typename ManagedDescriptor>
class Manager {
public:
    explicit Manager(fd_set& fdSet)
        : fdSet(fdSet) {
    }

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;

private:
    static bool contains(std::list<ManagedDescriptor*>& listOfElements, ManagedDescriptor*& element) {
        typename std::list<ManagedDescriptor*>::iterator it = std::find(listOfElements.begin(), listOfElements.end(), element);

        return it != listOfElements.end();
    }

public:
    void start(ManagedDescriptor* socket) {
        if (!socket->isManaged() && !Manager<ManagedDescriptor>::contains(addedDescriptors, socket)) {
            addedDescriptors.push_back(socket);
            socket->incManaged();
        }
    }

    void stop(ManagedDescriptor* socket) {
        if (socket->isManaged() && !Manager<ManagedDescriptor>::contains(removedDescriptors, socket)) {
            removedDescriptors.push_back(socket);
        }
    }

private:
    int getMaxFd() {
        int fd = 0;

        addDescriptors();
        removeDescriptors();

        if (!descriptors.empty()) {
            fd = descriptors.rbegin()->first;
        }

        return fd;
    }

    void addDescriptors() {
        if (!addedDescriptors.empty()) {
            for (ManagedDescriptor* descriptor : addedDescriptors) {
                int fd = dynamic_cast<Descriptor*>(descriptor)->getFd();
                FD_SET(fd, &fdSet);
                descriptors[fd] = descriptor;
            }
            addedDescriptors.clear();
        }
    }

    void removeDescriptors() {
        if (!removedDescriptors.empty()) {
            for (ManagedDescriptor* descriptor : removedDescriptors) {
                int fd = dynamic_cast<Descriptor*>(descriptor)->getFd();
                FD_CLR(fd, &fdSet);
                descriptors.erase(fd);
                descriptor->decManaged();
            }
            removedDescriptors.clear();
        }
    }

    void removeManagedDescriptors() {
        for (std::pair<int, ManagedDescriptor*> descriptor : descriptors) {
            removedDescriptors.push_back(descriptor.second);
        }

        removeDescriptors();
    }

protected:
    virtual int dispatch(const fd_set& fdSet, int count) = 0;

    std::map<int, ManagedDescriptor*> descriptors;

private:
    std::list<ManagedDescriptor*> addedDescriptors;
    std::list<ManagedDescriptor*> removedDescriptors;

    fd_set& fdSet;

public:
    using ManagedDescriptorType = ManagedDescriptor;

    friend class Multiplexer;
};

#endif // MANAGER_H
