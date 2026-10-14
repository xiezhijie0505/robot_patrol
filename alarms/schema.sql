-- 报警记录表（SQLite）
-- 图片以文件形式存放，本表只存相对路径（相对 data/images/）

CREATE TABLE IF NOT EXISTS alarms (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    created_at  TEXT    NOT NULL DEFAULT (datetime('now', 'localtime')),
    type        TEXT    NOT NULL,          -- smoke | stranger | other
    score       REAL,                     -- 置信度，可空
    image_rel   TEXT,                     -- 相对 data/images/，可空（如纯烟雾无图）
    note        TEXT                      -- 备注
);

CREATE INDEX IF NOT EXISTS idx_alarms_created_at ON alarms(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_alarms_type ON alarms(type);
