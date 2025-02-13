// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task/task_scheduler/scheduler_worker_pool.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/lazy_instance.h"
#include "base/task/task_scheduler/task_tracker.h"
#include "base/threading/thread_local.h"

namespace base {
namespace internal {

namespace {

// SchedulerWorkerPool that owns the current thread, if any.
LazyInstance<ThreadLocalPointer<const SchedulerWorkerPool>>::Leaky
    tls_current_worker_pool = LAZY_INSTANCE_INITIALIZER;

const SchedulerWorkerPool* GetCurrentWorkerPool() {
  return tls_current_worker_pool.Get().Get();
}

}  // namespace

SchedulerWorkerPool::SchedulerWorkerPool(
    TrackedRef<TaskTracker> task_tracker,
    TrackedRef<Delegate> delegate)
    : task_tracker_(std::move(task_tracker)),
      delegate_(std::move(delegate)) {
  DCHECK(task_tracker_);
}

SchedulerWorkerPool::~SchedulerWorkerPool() = default;

void SchedulerWorkerPool::BindToCurrentThread() {
  DCHECK(!GetCurrentWorkerPool());
  tls_current_worker_pool.Get().Set(this);
}

void SchedulerWorkerPool::UnbindFromCurrentThread() {
  DCHECK(GetCurrentWorkerPool());
  tls_current_worker_pool.Get().Set(nullptr);
}

bool SchedulerWorkerPool::IsBoundToCurrentThread() const {
  return GetCurrentWorkerPool() == this;
}

void SchedulerWorkerPool::PostTaskWithSequenceNow(
    Task task,
    std::unique_ptr<Sequence::Transaction> sequence_transaction) {
  DCHECK(task.task);
  DCHECK(sequence_transaction);

  // Confirm that |task| is ready to run (its delayed run time is either null or
  // in the past).
  DCHECK_LE(task.delayed_run_time, TimeTicks::Now());

  const bool sequence_was_empty =
      sequence_transaction->PushTask(std::move(task));
  if (sequence_was_empty) {
    // Try to schedule the Sequence locked by |sequence_transaction| if it was
    // empty before |task| was inserted into it. Otherwise, one of these must be
    // true:
    // - The Sequence is already scheduled, or,
    // - The pool is running a Task from the Sequence. The pool is expected to
    //   reschedule the Sequence once it's done running the Task.
    sequence_transaction = task_tracker_->WillScheduleSequence(
        std::move(sequence_transaction), this);
    if (sequence_transaction)
      OnCanScheduleSequence(std::move(sequence_transaction));
  }
}

}  // namespace internal
}  // namespace base
