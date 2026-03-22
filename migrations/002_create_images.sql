CREATE TABLE images
(
    id         INTEGER NOT NULL PRIMARY KEY,
    sender     TEXT    NOT NULL,
    image_path TEXT    NOT NULL,
    expires_at INTEGER NOT NULL
);
