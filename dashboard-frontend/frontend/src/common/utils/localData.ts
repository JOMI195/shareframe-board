// Cookies are HttpOnly, so the session itself survives; only what the browser keeps for the app goes.
export const clearLocalDataAndReload = async () => {
  try {
    localStorage.clear();
    sessionStorage.clear();
    if ('caches' in window) {
      const keys = await caches.keys();
      await Promise.all(keys.map((key) => caches.delete(key)));
    }
  } finally {
    window.location.reload();
  }
};
