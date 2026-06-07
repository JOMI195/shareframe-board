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
            const response = await fetchWithTimeout('/api/pi/check-connection');
            const data = await response.json();

            setIsConnected(data.connected);
            return data.connected;
        } catch (error) {
            console.error('Error checking Pi connection:', error);
            setIsConnected(false);
            return false;
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