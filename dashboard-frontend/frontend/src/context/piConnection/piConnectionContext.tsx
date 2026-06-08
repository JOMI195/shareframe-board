import { fetchWithTimeout } from '@/common/utils/fetch';
import React, { createContext, useState, useContext, useEffect, useCallback } from 'react';

interface PiConnectionContextType {
    isConnected: boolean;
    isCheckingConnection: boolean;
    checkPiConnection: () => Promise<void>;
}

const PiConnectionContext = createContext<PiConnectionContextType>({
    isConnected: false,
    isCheckingConnection: false,
    checkPiConnection: async () => { },
});

export const PiConnectionProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
    const [isConnected, setIsConnected] = useState<boolean>(false);
    const [isCheckingConnection, setIsCheckingConnection] = useState<boolean>(false);

    const checkPiConnection = useCallback(async () => {
        try {
            setIsCheckingConnection(true);
            // The dashboard *is* the board, so "reachable" == the server answered.
            const response = await fetchWithTimeout('/api/system/health');
            const payload = await response.json();

            const connected = !!payload?.data?.running;
            setIsConnected(connected);
        } catch (error) {
            console.error('Error checking Pi connection:', error);
            setIsConnected(false);
        } finally {
            setIsCheckingConnection(false);
        }
    }, []);

    // Periodically check connection
    useEffect(() => {
        checkPiConnection();

        const intervalId = setInterval(checkPiConnection, 30000);

        return () => clearInterval(intervalId);
    }, [checkPiConnection]);

    return (
        <PiConnectionContext.Provider
            value={{
                isConnected,
                isCheckingConnection,
                checkPiConnection
            }}
        >
            {children}
        </PiConnectionContext.Provider>
    );
};

export const usePiConnection = () => {
    const context = useContext(PiConnectionContext);

    if (!context) {
        throw new Error('usePiConnection must be used within a PiConnectionProvider');
    }

    return context;
};