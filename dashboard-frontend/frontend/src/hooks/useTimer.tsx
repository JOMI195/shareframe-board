import { useEffect, useRef } from 'react';
import { RootState, useAppDispatch, useAppSelector } from '../store';
import { addTimer, formatTime, removeTimer, resetTimer, selectTimer, startTimer, stopTimer, syncSpecificTimer } from '@/store/timers/timers.Slice';

interface UseTimerOptions {
    id: string;
    duration?: number;
    autoStart?: boolean;
    shouldCreate?: boolean;
}

export const useTimer = ({ id, duration = 180, autoStart = false, shouldCreate = true }: UseTimerOptions) => {
    const dispatch = useAppDispatch();
    const timer = useAppSelector((state: RootState) => selectTimer(state, id));
    const intervalRef = useRef<NodeJS.Timeout | null>(null);

    // This ensures the timer is properly initialized
    useEffect(() => {
        if (!timer && shouldCreate) {
            // Create timer if it doesn't exist
            dispatch(addTimer({ id, duration }));

            if (autoStart) {
                setTimeout(() => dispatch(startTimer(id)), 0);
            }
        } else {
            // If timer exists but was created elsewhere, sync it immediately
            dispatch(syncSpecificTimer(id));
        }
    }, [id, duration, autoStart, timer, dispatch]);

    // Set up the interval for active timers with improved cleanup
    useEffect(() => {
        // Clear any existing interval
        if (intervalRef.current) {
            clearInterval(intervalRef.current);
            intervalRef.current = null;
        }

        // Only set interval if timer exists and is active
        if (timer?.isActive) {
            // Initial sync
            dispatch(syncSpecificTimer(id));

            // Set up interval to sync every second
            intervalRef.current = setInterval(() => {
                dispatch(syncSpecificTimer(id));
            }, 1000);
        }

        // Cleanup on unmount
        return () => {
            if (intervalRef.current) {
                clearInterval(intervalRef.current);
            }
        };
    }, [id, timer?.isActive, dispatch]);

    // Additional effect to handle window focus/blur
    useEffect(() => {
        // Sync on window focus to handle if the page was in background
        const handleFocus = () => dispatch(syncSpecificTimer(id));
        window.addEventListener('focus', handleFocus);

        return () => {
            window.removeEventListener('focus', handleFocus);
        };
    }, [id, dispatch]);

    const start = () => {
        if (timer && !timer.isActive) {
            dispatch(startTimer(id));
        }
    };

    const stop = () => {
        if (timer?.isActive) {
            dispatch(stopTimer(id));
        }
    };

    const reset = () => {
        dispatch(resetTimer(id));
    };

    const remove = () => {
        dispatch(removeTimer(id));
    };

    return {
        time: timer?.remaining || 0,
        formattedTime: formatTime(timer?.remaining || 0),
        isActive: timer?.isActive || false,
        start,
        stop,
        reset,
        remove,
        isComplete: timer ? timer.remaining <= 0 : false
    };
};