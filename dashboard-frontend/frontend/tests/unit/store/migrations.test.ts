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

describe('migration 5', () => {
  it('keeps only what should survive a reload', () => {
    const before = { ...rootState({ navigation: { advancedMode: true } }), timers: { timers: {} } };

    const after = migrations[5](before as never) as unknown as Record<string, unknown>;

    expect(Object.keys(after).sort()).toEqual(['_persist', 'loadingWall', 'navigation']);
    expect((after as ReturnType<typeof rootState>).navigation.advancedMode).toBe(true);
  });
});
