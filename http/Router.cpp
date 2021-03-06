#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Router.h"


static inline std::string path_concat(const std::string& first, const std::string& second) {
    std::string result;

    if (first.back() == '/' && second.front() == '/') {
        result = first + second.substr(1);
    } else if (first.back() != '/' && second.front() != '/') {
        result = first + '/' + second;
    } else {
        result = first + second;
    }

    if (result.length() > 1 && result.back() == '/') {
        result.pop_back();
    }

    return result;
}


class Dispatcher {
public:
    Dispatcher() = default;
    Dispatcher(const Dispatcher&) = delete;

    Dispatcher& operator=(const Dispatcher&) = delete;

    virtual ~Dispatcher() = default;

    [[nodiscard]] virtual bool dispatch(const MountPoint& mountPoint, const std::string& parentPath, Request& req, Response& res) const = 0;
};


class Route {
public:
    Route(Router* parent, const std::string& method, const std::string& path, const std::shared_ptr<Dispatcher>& route)
        : parent(parent)
        , mountPoint(method, path)
        , route(route) {
    }

    [[nodiscard]] bool dispatch(const std::string& parentPath, Request& req, Response& res) const;

private:
    Router* parent;
    MountPoint mountPoint;
    std::shared_ptr<Dispatcher> route;
};


class RouterDispatcher : public Dispatcher {
public:
    explicit RouterDispatcher() = default;

    [[nodiscard]] bool dispatch(const MountPoint& mountPoint, const std::string& parentPath, Request& req, Response& res) const override;

private:
    std::list<Route> routes;

    friend class Router;
};


class MiddlewareDispatcher : public Dispatcher {
public:
    explicit MiddlewareDispatcher(const std::function<void(Request& req, Response& res, const std::function<void(void)>& next)>& dispatcher)
        : dispatcher(dispatcher) {
    }

    [[nodiscard]] bool dispatch(const MountPoint& mountPoint, const std::string& parentPath, Request& req, Response& res) const override;

private:
    const std::function<void(Request& req, Response& res, std::function<void(void)>)> dispatcher;
};


class RouteDispatcher : public Dispatcher {
public:
    explicit RouteDispatcher(const std::function<void(Request& req, Response& res)>& dispatcher)
        : dispatcher(dispatcher) {
    }

    [[nodiscard]] bool dispatch(const MountPoint& mountPoint, const std::string& parentPath, Request& req, Response& res) const override;

protected:
    const std::function<void(Request& req, Response& res)> dispatcher;
};


bool Route::dispatch(const std::string& parentPath, Request& req, Response& res) const {
    return route->dispatch(mountPoint, parentPath, req, res);
}


bool RouterDispatcher::dispatch(const MountPoint& mountPoint, const std::string& parentPath, Request& req, Response& res) const {
    bool next = true;
    std::string cpath = path_concat(parentPath, mountPoint.path);

    if (req.path.rfind(cpath, 0) == 0 && (mountPoint.method == req.method || mountPoint.method == "use")) {
        req.url = req.originalUrl.substr(cpath.length());

        if (req.url.front() != '/') {
            req.url.insert(0, "/");
        }

        std::list<Route>::const_iterator route = routes.begin();
        std::list<Route>::const_iterator end = routes.end();

        while (route != end && next) { // todo: to be exchanged by an stl-algorithm
            next = route->dispatch(cpath, req, res);
            ++route;
        }
    }
    return next;
}


bool MiddlewareDispatcher::dispatch(const MountPoint& mountPoint, const std::string& parentPath, Request& req, Response& res) const {
    bool next = true;
    std::string cpath = path_concat(parentPath, mountPoint.path);

    if ((req.path.rfind(cpath, 0) == 0 && mountPoint.method == "use") ||
        (cpath == req.path && (req.method == mountPoint.method || mountPoint.method == "all"))) {
        req.url = req.originalUrl.substr(cpath.length());

        if (req.url.front() != '/') {
            req.url.insert(0, "/");
        }

        next = false;
        this->dispatcher(req, res, [&next]() -> void {
            next = true;
        });
    }
    return next;
}


bool RouteDispatcher::dispatch(const MountPoint& mountPoint, const std::string& parentPath, Request& req, Response& res) const {
    bool next = true;
    std::string cpath = path_concat(parentPath, mountPoint.path);

    if (cpath == req.path && (req.method == mountPoint.method || mountPoint.method == "all")) {
        req.url = req.originalUrl.substr(cpath.length());

        if (req.url.front() != '/') {
            req.url.insert(0, "/");
        }

        this->dispatcher(req, res);

        next = false;
    }

    return next;
}


Router::Router()
    : mountPoint("use", "/")
    , routerRoute(new RouterDispatcher()) {
}


void Router::dispatch(Request& req, Response& res) const {
    [[maybe_unused]] bool next = routerRoute->dispatch(mountPoint, "/", req, res);
}


#define REQUESTMETHOD(METHOD, HTTP_METHOD)                                                                                                 \
    Router& Router::METHOD(const std::string& path, const std::function<void(Request & req, Response & res)>& dispatcher) {                \
        routerRoute->routes.emplace_back(Route(this, HTTP_METHOD, path, std::make_shared<RouteDispatcher>(dispatcher)));                   \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& Router::METHOD(const std::function<void(Request & req, Response & res)>& dispatcher) {                                         \
        routerRoute->routes.emplace_back(Route(this, HTTP_METHOD, "", std::make_shared<RouteDispatcher>(dispatcher)));                     \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& Router::METHOD(const std::string& path, const Router& router) {                                                                \
        routerRoute->routes.emplace_back(Route(this, HTTP_METHOD, path, router.routerRoute));                                              \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& Router::METHOD(const Router& router) {                                                                                         \
        routerRoute->routes.emplace_back(Route(this, HTTP_METHOD, "", router.routerRoute));                                                \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& Router::METHOD(const std::string& path,                                                                                        \
                           const std::function<void(Request & req, Response & res, const std::function<void(void)>& next)>& dispatcher) {  \
        routerRoute->routes.emplace_back(Route(this, HTTP_METHOD, path, std::make_shared<MiddlewareDispatcher>(dispatcher)));              \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& Router::METHOD(const std::function<void(Request & req, Response & res, const std::function<void(void)>& next)>& dispatcher) {  \
        routerRoute->routes.emplace_back(Route(this, HTTP_METHOD, "", std::make_shared<MiddlewareDispatcher>(dispatcher)));                \
        return *this;                                                                                                                      \
    };


REQUESTMETHOD(use, "use");
REQUESTMETHOD(all, "all");
REQUESTMETHOD(get, "GET");
REQUESTMETHOD(put, "PUT");
REQUESTMETHOD(post, "POST");
REQUESTMETHOD(del, "DELETE");
REQUESTMETHOD(connect, "CONNECT");
REQUESTMETHOD(options, "OPTIONS");
REQUESTMETHOD(trace, "TRACE");
REQUESTMETHOD(patch, "PATCH");
REQUESTMETHOD(head, "HEAD");
