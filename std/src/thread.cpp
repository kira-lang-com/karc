// The cooperative scheduler behind <thread>, <condition_variable> and
// <future>: a queue of tasks, each run to completion on the one thread.
#include <__thread_tasks>
#include <cstdlib>

static std::karc_task** queue;
static unsigned long queued;
static unsigned long room;
static unsigned long first;
static unsigned long nextId = 1;
static unsigned long current;

unsigned long karc_task_start(std::karc_task* task) {
    if (first == queued) {
        first = 0;
        queued = 0;
    }
    if (queued == room) {
        if (room > (unsigned long)-1 / 2) { abort(); }
        unsigned long grownRoom = room == 0 ? 16 : room * 2;
        if (grownRoom > (unsigned long)-1 / sizeof(std::karc_task*)) { abort(); }
        std::karc_task** grown = (std::karc_task**)realloc(queue, grownRoom * sizeof(std::karc_task*));
        if (grown == 0) { abort(); }
        queue = grown;
        room = grownRoom;
    }
    queue[queued] = task;
    queued++;
    task->id_ = nextId;
    nextId++;
    return task->id_;
}

static void runTask(std::karc_task* task) {
    unsigned long saved = current;
    current = task->id_;
    task->started_ = true;
    task->run();
    task->finished_ = true;
    current = saved;
}

bool karc_task_run_one() {
    while (first < queued) {
        std::karc_task* task = queue[first];
        first++;
        if (task != 0 && !task->started_) {
            runTask(task);
            return true;
        }
    }
    return false;
}

void karc_task_finish(std::karc_task* task) {
    if (task->started_) { return; }
    for (unsigned long i = first; i < queued; i++) {
        if (queue[i] == task) { queue[i] = 0; }
    }
    runTask(task);
}

unsigned long karc_task_current() { return current; }
