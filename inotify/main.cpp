#include "inotify/inotifyBuilder.h"

#include <boost/filesystem.hpp>

#include <iostream>
#include <thread>
#include <chrono>

using namespace inotify;

int main(int argc, char** argv)
{
    if (argc <= 1) {
        std::cout << "Usage: ./inotify_example /path/to/dir" << std::endl;
        exit(0);
    }

    // Parse the directory to watch
    boost::filesystem::path path(argv[1]);

    auto handleEvent = [&] (InotifyEvent inotifyEvent) {
        std::cout << "Event " << inotifyEvent.event << " on " << inotifyEvent.path << 
            " was triggered." << std::endl;
    };
    auto handleMoveEvent = [&] (InotifyEvent inotifyEvent1, InotifyEvent inotifyEvent2) {
        std::cout << "Event " << inotifyEvent1.event << " " << inotifyEvent1.path << " " << 
            inotifyEvent2.event << " " << inotifyEvent2.path << std::endl;
    };
    auto handleUnexpectedEvent = [&] (InotifyEvent inotifyEvent) {
        std::cout << "Event " << inotifyEvent.event << " on " << inotifyEvent.path << 
            " was triggered, but was not expected." << std::endl;
    };

    auto events = {
        Event::create,
        Event::create | Event::is_dir,
        Event::modify,
        Event::remove,
        Event::remove | Event::is_dir,
        Event::moved_from,
        Event::moved_to
    };

    auto notifier = BuildInotify()
                        .onEvents(events, handleEvent)
                        .onMoveEvent(handleMoveEvent)
                        .onUnexpectedEvent(handleUnexpectedEvent)
                        .ignoreFile(path / "a")
                        .watchpathRecursively(path);
        
    std::thread thread([&](){ notifier.run(); });
    std::this_thread::sleep_for(std::chrono::seconds(600));
    notifier.stop();
    thread.join();
    return 0;
}
