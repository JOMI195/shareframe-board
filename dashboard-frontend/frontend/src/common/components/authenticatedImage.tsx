import { Skeleton } from '@mui/material';
import React, { useState, useEffect, useRef } from 'react';
import { useMinimumLoading } from '@/hooks/loading/useMinimumLoading';
import { fetchWithTimeout } from '@/common/utils/fetch';

interface AuthenticatedImageProps {
    url: string;
    alt: string;
    className?: string;
    style?: React.CSSProperties;
    onClick?: () => void;
    onError?: (error: Error) => void;
    aspectRatio?: number;
}

const AuthenticatedImage: React.FC<AuthenticatedImageProps> = ({
    url,
    alt,
    className,
    style,
    onClick,
    onError,
    aspectRatio
}) => {
    const [imageSrc, setImageSrc] = useState<string>('');
    const [isLoading, setIsLoading] = useState<boolean>(true);
    const [isVisible, setIsVisible] = useState<boolean>(false);
    const imgRef = useRef<HTMLDivElement>(null);
    const showSkeleton = useMinimumLoading(isLoading);

    useEffect(() => {
        const observer = new IntersectionObserver(
            ([entry]) => {
                if (entry.isIntersecting) {
                    setIsVisible(true);
                }
            },
            {
                rootMargin: '100px', // Preload when 100px away from viewport
                threshold: 0.1
            }
        );

        const element = imgRef.current;
        if (element) {
            observer.observe(element);
        }

        return () => {
            if (element) {
                observer.unobserve(element);
            }
        };
    }, []);

    useEffect(() => {
        if (!isVisible) return;

        let isMounted = true;
        let objectUrl = '';

        const loadImage = async (): Promise<void> => {
            try {
                setIsLoading(true);
                const response = await fetchWithTimeout(url);
                if (!response.ok) {
                    throw new Error(`Failed to load image (${response.status})`);
                }
                objectUrl = URL.createObjectURL(await response.blob());

                if (isMounted) {
                    setImageSrc(objectUrl);
                    setIsLoading(false);
                } else {
                    URL.revokeObjectURL(objectUrl);
                }
            } catch (error) {
                if (!isMounted) return;
                setIsLoading(false);
                if (error instanceof Error) {
                    onError?.(error);
                } else {
                    onError?.(new Error('Failed to load image'));
                }
            }
        };

        loadImage();

        return () => {
            isMounted = false;
            if (objectUrl) {
                URL.revokeObjectURL(objectUrl);
            }
        };
    }, [url, isVisible, onError]);

    if (showSkeleton) {
        // A hidden img of the same ratio reserves the box: percentage widths collapse when the parent is shrink-to-fit.
        if (aspectRatio) {
            const ratioSrc = `data:image/svg+xml;utf8,${encodeURIComponent(
                `<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="${Math.round(1000 / aspectRatio)}"/>`
            )}`;
            return (
                <div ref={imgRef} style={{ position: 'relative', display: 'inline-block', overflow: 'hidden', ...style }}>
                    <img src={ratioSrc} alt="" style={{ ...style, display: 'block', visibility: 'hidden' }} />
                    <Skeleton
                        variant="rectangular"
                        animation="wave"
                        style={{ position: 'absolute', inset: 0, borderRadius: style?.borderRadius }}
                    />
                </div>
            );
        }
        return (
            <Skeleton
                variant="rectangular"
                animation="wave"
                style={{ width: '100%', height: '100%', ...style }}
                ref={imgRef}
            />
        );
    }

    return (
        <div ref={imgRef} style={{ overflow: 'hidden', ...style }}>
            {imageSrc ? (
                <img
                    src={imageSrc}
                    alt={alt}
                    className={className}
                    loading="lazy"
                    style={{
                        ...style,
                        transition: 'opacity 0.5s ease-in-out',
                        opacity: isLoading ? 0 : 1,
                    }}
                    onClick={onClick}
                />
            ) : null}
        </div>
    );
};

export default AuthenticatedImage;
