import React, { ReactNode } from 'react';
import FinalError from '../finalError/finalError';
import LoadingFallback from '../../loadingFallback';
import StandaloneTheme from '../standaloneTheme';

interface ErrorBoundaryProps {
    children: ReactNode;
}

interface ErrorBoundaryState {
    hasError: boolean;
    showContactPage: boolean;
}

const ERROR_STORAGE_KEY = 'error_count';

class ErrorBoundary extends React.Component<ErrorBoundaryProps, ErrorBoundaryState> {
    constructor(props: ErrorBoundaryProps) {
        super(props);
        this.state = {
            hasError: false,
            showContactPage: false
        };
    }

    // Log error to localStorage
    private logError = () => {
        const errorLog = JSON.parse(localStorage.getItem(ERROR_STORAGE_KEY) || '[]');
        errorLog.push({
            timestamp: new Date().toISOString()
        });

        // Limit to last 10 errors
        localStorage.setItem(ERROR_STORAGE_KEY, JSON.stringify(errorLog.slice(-5)));
    };

    // Count errors from log
    private getErrorCount = (): number => {
        const errorLog = JSON.parse(localStorage.getItem(ERROR_STORAGE_KEY) || '[]');
        return errorLog.length;
    };

    // Clear error logs
    private clearErrorLogs = () => {
        localStorage.removeItem(ERROR_STORAGE_KEY);
    };

    // Lifecycle method to catch errors
    componentDidCatch(error: Error, errorInfo: React.ErrorInfo) {
        // Log the error to an error reporting service
        if (import.meta.env.VITE_APP_PRODUCTION === "False") {
            console.error("Caught error:", error, errorInfo);
        }

        this.setState({
            hasError: true,
        });

        // Count errors from log
        const errorCount = this.getErrorCount();

        this.logError();

        // On first error: reload page
        if (errorCount < 1) {
            setTimeout(() => {
                localStorage.removeItem("persist:shareframe-dashboard-data");
                window.location.reload();
            }, 2000);
        } else {
            this.setState({
                hasError: true,
                showContactPage: true
            });
        }
    }

    render() {
        // If there's a repeated error, show contact page
        if (this.state.showContactPage) {
            // Clear error logs when showing final error page
            this.clearErrorLogs();
            return <FinalError />;
        }

        // If there's an error but not yet showing contact page
        if (this.state.hasError && !this.state.showContactPage) {
            return (<StandaloneTheme><LoadingFallback /></StandaloneTheme>);
        }

        // Render children normally
        return this.props.children;
    }
}

export default ErrorBoundary;