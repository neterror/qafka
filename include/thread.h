#pragma once

namespace Thread {
    template<typename T>
    T* _start(T* worker) {
        auto thread = new QThread;
        worker->moveToThread(thread);

        QObject::connect(thread, &QThread::started,  worker, &T::work);
        QObject::connect(worker, &T::finished,       thread, &QThread::quit);
        QObject::connect(worker, &T::finished,       worker, &QObject::deleteLater);
        QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
        thread->start();
        
        return worker;
    }

    template<typename T>
    T* spawn() {
        return _start(new T());
    }

    template<typename T, typename P>
    T* spawn(P p) {
        return _start(new T(p));
    }
}

