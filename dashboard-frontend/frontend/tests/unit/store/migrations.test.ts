import { describe, expect, it } from 'vitest';
import migrations from '@/store/migrations';
import { rootState } from '@tests/helpers/preloadedState';

describe.each([3, 4])('migration %i', (version) => {
  const migrate = migrations[version];

  it('drops the persisted dialogs so the slice re-initialises', () => {
    const after = migrate(rootState()) as unknown as Record<string, unknown>;

    expect(after).not.toHaveProperty('dialogs');
  });

  it('keeps every other slice', () => {
    const before = rootState({ navigation: { advancedMode: true } });

    const after = migrate(before) as unknown as ReturnType<typeof rootState>;

    expect(after.navigation.advancedMode).toBe(true);
    expect(after._persist).toEqual(before._persist);
  });

  it('passes an empty state through', () => {
    expect(migrate(undefined)).toBeUndefined();
  });
});
