export const getBadgeNumber = (...conditions: boolean[]): number => {
    return conditions.filter(Boolean).length;
};