-- ============================================
-- 设备数据服务 MySQL 数据库初始化脚本
-- ============================================

-- 创建数据库（如果不存在）
CREATE DATABASE IF NOT EXISTS device_data 
    DEFAULT CHARACTER SET utf8mb4 
    DEFAULT COLLATE utf8mb4_unicode_ci;

USE device_data;

-- ============================================
-- 设备表
-- 存储已注册的设备信息
-- ============================================
CREATE TABLE IF NOT EXISTS devices (
    id BIGINT AUTO_INCREMENT PRIMARY KEY COMMENT '自增主键',
    device_id VARCHAR(128) NOT NULL COMMENT '设备唯一标识',
    device_name VARCHAR(256) DEFAULT NULL COMMENT '设备名称（可选）',
    device_type VARCHAR(64) DEFAULT NULL COMMENT '设备类型（可选）',
    status TINYINT DEFAULT 1 COMMENT '设备状态：0-离线，1-在线',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    
    -- 唯一索引：确保 device_id 唯一
    UNIQUE INDEX idx_device_id (device_id),
    -- 状态索引：便于查询在线/离线设备
    INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='设备表';

-- ============================================
-- 数据点表
-- 存储设备上报的时序数据
-- ============================================
CREATE TABLE IF NOT EXISTS data_points (
    id BIGINT AUTO_INCREMENT PRIMARY KEY COMMENT '自增主键',
    device_id VARCHAR(128) NOT NULL COMMENT '设备ID',
    timestamp BIGINT NOT NULL COMMENT 'Unix时间戳（秒或毫秒）',
    metrics JSON NOT NULL COMMENT '指标数据（JSON格式）',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '入库时间',
    
    -- 联合索引：按设备ID和时间戳查询（降序优化最近数据查询）
    INDEX idx_device_timestamp (device_id, timestamp DESC),
    -- 时间戳索引：用于按时间范围清理数据
    INDEX idx_timestamp (timestamp),
    -- 入库时间索引：用于按入库时间清理数据
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
  COMMENT='数据点表';

-- ============================================
-- 可选：数据点分区表（用于大数据量场景）
-- 按月分区，便于数据归档和清理
-- ============================================
-- CREATE TABLE IF NOT EXISTS data_points_partitioned (
--     id BIGINT AUTO_INCREMENT,
--     device_id VARCHAR(128) NOT NULL,
--     timestamp BIGINT NOT NULL,
--     metrics JSON NOT NULL,
--     created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
--     
--     PRIMARY KEY (id, created_at),
--     INDEX idx_device_timestamp (device_id, timestamp DESC),
--     INDEX idx_timestamp (timestamp)
-- ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
--   PARTITION BY RANGE (TO_DAYS(created_at)) (
--     PARTITION p202601 VALUES LESS THAN (TO_DAYS('2026-02-01')),
--     PARTITION p202602 VALUES LESS THAN (TO_DAYS('2026-03-01')),
--     PARTITION p202603 VALUES LESS THAN (TO_DAYS('2026-04-01')),
--     PARTITION p_future VALUES LESS THAN MAXVALUE
-- );

-- ============================================
-- 存储过程：清理指定天数之前的数据
-- ============================================
DELIMITER //

CREATE PROCEDURE IF NOT EXISTS cleanup_old_data(IN days_to_keep INT)
BEGIN
    DECLARE cutoff_timestamp BIGINT;
    DECLARE deleted_rows INT DEFAULT 0;
    
    -- 计算截止时间戳（秒）
    SET cutoff_timestamp = UNIX_TIMESTAMP(DATE_SUB(NOW(), INTERVAL days_to_keep DAY));
    
    -- 分批删除，避免长时间锁表
    REPEAT
        DELETE FROM data_points 
        WHERE timestamp < cutoff_timestamp 
        LIMIT 10000;
        
        SET deleted_rows = ROW_COUNT();
        
        -- 短暂休眠，减少对在线业务的影响
        DO SLEEP(0.1);
        
    UNTIL deleted_rows = 0
    END REPEAT;
    
    SELECT CONCAT('Cleanup completed. Deleted data before timestamp: ', cutoff_timestamp) AS result;
END //

DELIMITER ;

-- ============================================
-- 事件调度器：自动清理30天前的数据（可选）
-- 需要先开启事件调度器：SET GLOBAL event_scheduler = ON;
-- ============================================
-- CREATE EVENT IF NOT EXISTS evt_cleanup_old_data
-- ON SCHEDULE EVERY 1 DAY
-- STARTS CURRENT_DATE + INTERVAL 3 HOUR
-- DO CALL cleanup_old_data(30);

-- ============================================
-- 查询示例
-- ============================================

-- 1. 查询指定设备最近100条数据
-- SELECT timestamp, metrics 
-- FROM data_points 
-- WHERE device_id = 'ECG_10086' 
-- ORDER BY timestamp DESC 
-- LIMIT 100;

-- 2. 查询指定时间范围的数据
-- SELECT timestamp, metrics 
-- FROM data_points 
-- WHERE device_id = 'ECG_10086' 
--   AND timestamp >= 1700000000 
--   AND timestamp <= 1700100000 
-- ORDER BY timestamp ASC;

-- 3. 统计每个设备的数据条数
-- SELECT device_id, COUNT(*) as count 
-- FROM data_points 
-- GROUP BY device_id 
-- ORDER BY count DESC;

-- 4. 查询所有在线设备
-- SELECT device_id, device_name, created_at 
-- FROM devices 
-- WHERE status = 1;

-- ============================================
-- 初始化完成
-- ============================================
SELECT 'Database initialization completed!' AS message;
