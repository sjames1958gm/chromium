// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tracing;

import android.content.Context;
import android.support.annotation.IntDef;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ObserverList;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.R;
import org.chromium.content_public.browser.TracingControllerAndroid;
import org.chromium.ui.widget.Toast;

import java.io.File;
import java.io.IOException;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;
import java.util.TimeZone;

/**
 * Coordinates recording and saving/sharing Chrome performance traces.
 */
public class TracingController {
    /**
     * Observer that is notified when the controller's tracing state changes.
     */
    public interface Observer {
        /**
         * Called by the TracingController when its state changes.
         *
         * @param state the new state of the Controller.
         */
        void onTracingStateChanged(@State int state);
    }

    /**
     * State of the controller. There can only be one active tracing session at the same time.
     */
    @IntDef({State.INITIALIZING, State.IDLE, State.STARTING, State.RECORDING, State.STOPPING,
            State.STOPPED})
    @Retention(RetentionPolicy.SOURCE)
    public @interface State {
        int INITIALIZING = 0;
        int IDLE = 1;
        int STARTING = 2;
        int RECORDING = 3;
        int STOPPING = 4;
        int STOPPED = 5;
    }

    private static final String TAG = "TracingController";
    private static final String TEMP_FILE_DIR = "/traces";
    private static final String TEMP_FILE_PREFIX = "chrome-trace-";
    private static final String TEMP_FILE_EXT = ".json.gz";

    private static TracingController sInstance;

    // Only set while a trace is in progress to avoid leaking native resources.
    private TracingControllerAndroid mNativeController;

    private ObserverList<Observer> mObservers = new ObserverList<>();
    private @State int mState = State.INITIALIZING;
    private Set<String> mKnownCategories;
    private File mTracingTempFile;

    private TracingController() {}

    /**
     * @return the singleton instance of TracingController, creating and initializing it if needed.
     */
    public static TracingController getInstance() {
        if (sInstance == null) {
            sInstance = new TracingController();
            sInstance.initialize();
        }
        return sInstance;
    }

    private void initialize() {
        mNativeController = TracingControllerAndroid.create(ContextUtils.getApplicationContext());
        mNativeController.getKnownCategories(categories -> {
            mKnownCategories = new HashSet<>(Arrays.asList(categories));

            // Also cleans up the controller.
            setState(State.IDLE);
        });
    }

    /**
     * Add the given observer to the controller's observer list.
     *
     * @param obs the observer to add.
     */
    public void addObserver(Observer obs) {
        mObservers.addObserver(obs);
    }

    /**
     * Remove the given observer from the controller's observer list.
     *
     * @param obs the observer to add.
     */
    public void removeObserver(Observer obs) {
        mObservers.removeObserver(obs);
    }

    /**
     * @return the current state of the controller.
     */
    public @State int getState() {
        return mState;
    }

    /**
     * @return the temporary file that the trace is written into.
     */
    @VisibleForTesting
    public File getTracingTempFile() {
        return mTracingTempFile;
    }

    /**
     * Should only be called after the controller was initialized.
     * @see TracingController#getState()
     * @see Observer#onTracingStateChanged(int)
     *
     * @return the set of known tracing categories.
     */
    public Set<String> getKnownCategories() {
        assert mKnownCategories != null;
        return mKnownCategories;
    }

    /**
     * Starts recording a trace into a temporary file and shows a persistent tracing notification.
     * Should only be called when in IDLE state.
     */
    public void startRecording() {
        assert mState == State.IDLE;
        assert mNativeController == null;
        assert TracingNotificationManager.browserNotificationsEnabled();

        mNativeController = TracingControllerAndroid.create(ContextUtils.getApplicationContext());

        setState(State.STARTING);
        TracingNotificationManager.showTracingActiveNotification();

        new CreateTempFileAndStartTraceTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private class CreateTempFileAndStartTraceTask extends AsyncTask<File> {
        @Override
        protected File doInBackground() {
            File cacheDir =
                    new File(ContextUtils.getApplicationContext().getCacheDir() + TEMP_FILE_DIR);
            cacheDir.mkdir();
            File tracingTempFile;
            SimpleDateFormat formatter = new SimpleDateFormat("yyyy-MM-dd-HHmmss", Locale.US);
            formatter.setTimeZone(TimeZone.getTimeZone("UTC"));
            try {
                tracingTempFile = new File(
                        cacheDir, TEMP_FILE_PREFIX + formatter.format(new Date()) + TEMP_FILE_EXT);
                tracingTempFile.createNewFile();
            } catch (IOException e) {
                Log.e(TAG, "Couldn't create chrome-trace temp file: %s", e.getMessage());
                return null;
            }
            return tracingTempFile;
        }

        @Override
        protected void onPostExecute(File result) {
            if (result == null) {
                showErrorToast();
                setState(State.IDLE);
                return;
            }
            mTracingTempFile = result;
            startNativeTrace();
        }
    }

    private void startNativeTrace() {
        assert mState == State.STARTING;

        // TODO(eseckler): Support configuring these.
        String categories = "*";
        String options = "record-until-full";

        if (!mNativeController.startTracing(
                    mTracingTempFile.getPath(), false, categories, options, true)) {
            Log.e(TAG, "Native error while trying to start tracing");
            showErrorToast();
            setState(State.IDLE);
            return;
        }

        setState(State.RECORDING);
    }

    /**
     * Stops an active trace recording and updates the tracing notification with stopping status.
     * Should only be called when in RECORDING state.
     */
    public void stopRecording() {
        assert mState == State.RECORDING;

        setState(State.STOPPING);
        TracingNotificationManager.showTracingStoppingNotification();

        mNativeController.stopTracing((Void v) -> {
            assert mState == State.STOPPING;

            setState(State.STOPPED);
            TracingNotificationManager.showTracingCompleteNotification();
        });
    }

    // TODO(eseckler): Add a way to download and/or share the trace.

    /**
     * Discards a recorded trace and cleans up the temporary trace file.
     * Should only be called when in STOPPED state.
     */
    public void discardTrace() {
        assert mState == State.STOPPED;
        // Setting the state also cleans up the temp file.
        setState(State.IDLE);
    }

    private void setState(@State int state) {
        Log.d(TAG, "State changing to %d", state);
        mState = state;
        if (mState == State.IDLE) {
            TracingNotificationManager.dismissNotification();
            if (mTracingTempFile != null) {
                new DeleteTempFileTask(mTracingTempFile)
                        .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                mTracingTempFile = null;
            }

            // Clean up the controller while idle to avoid leaking native resources.
            mNativeController.destroy();
            mNativeController = null;
        }
        for (Observer obs : mObservers) {
            obs.onTracingStateChanged(state);
        }
    }

    private class DeleteTempFileTask extends AsyncTask<Void> {
        private File mTracingTempFile;

        public DeleteTempFileTask(File tracingTempFile) {
            mTracingTempFile = tracingTempFile;
        }

        @Override
        protected Void doInBackground() {
            mTracingTempFile.delete();
            return null;
        }
    }

    private void showErrorToast() {
        Context context = ContextUtils.getApplicationContext();
        Toast.makeText(context, context.getString(R.string.tracing_error_toast), Toast.LENGTH_SHORT)
                .show();
    }
}
