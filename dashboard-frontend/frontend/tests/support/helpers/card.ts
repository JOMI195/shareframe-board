import { screen } from '@testing-library/react';

const cardOf = (heading: HTMLElement) => heading.closest('.MuiCard-root') as HTMLElement;

/** The ShareframeInfoCard titled `title`, for scoping queries with within(). */
export const card = (title: string) => cardOf(screen.getByRole('heading', { name: title }));

export const findCard = async (title: string) => cardOf(await screen.findByRole('heading', { name: title }));
