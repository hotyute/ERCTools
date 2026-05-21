CREATE DATABASE IF NOT EXISTS erc_tools CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE erc_tools;

CREATE TABLE IF NOT EXISTS users (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(128) NOT NULL UNIQUE,
    display_name VARCHAR(255) NOT NULL,
    position ENUM('Administrator', 'Supervisor', 'Manager', 'ERC') NOT NULL,
    pod ENUM('Pod 1', 'Pod 2', 'Pod 3', 'Pod 4', 'Pod 5', 'Pod 6', 'Pod 7', 'Pod 8', 'Pod 9') NOT NULL,
    password_salt CHAR(32) NOT NULL,
    password_hash CHAR(64) NOT NULL,
    password_iterations INT NOT NULL DEFAULT 150000,
    active TINYINT(1) NOT NULL DEFAULT 1,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    last_login_at TIMESTAMP NULL DEFAULT NULL
);

CREATE TABLE IF NOT EXISTS user_sessions (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    token_hash CHAR(64) NOT NULL UNIQUE,
    user_id BIGINT UNSIGNED NOT NULL,
    session_display_name VARCHAR(255) NULL DEFAULT NULL,
    session_position VARCHAR(32) NULL DEFAULT NULL,
    session_pod VARCHAR(32) NULL DEFAULT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    last_seen_at TIMESTAMP NULL DEFAULT NULL,
    expires_at TIMESTAMP NOT NULL,
    INDEX idx_user_sessions_user (user_id),
    INDEX idx_user_sessions_expires (expires_at),
    INDEX idx_user_sessions_pod_seen (session_pod, last_seen_at),
    CONSTRAINT fk_user_sessions_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS chat_messages (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    user_id BIGINT UNSIGNED NOT NULL,
    author VARCHAR(255) NOT NULL,
    body TEXT NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_chat_messages_created (created_at),
    CONSTRAINT fk_chat_messages_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS map_notes (
    id VARCHAR(64) NOT NULL PRIMARY KEY,
    user_id BIGINT UNSIGNED NOT NULL,
    author VARCHAR(255) NOT NULL,
    body TEXT NOT NULL,
    latitude DOUBLE NOT NULL,
    longitude DOUBLE NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_by BIGINT UNSIGNED NULL DEFAULT NULL,
    deleted_at TIMESTAMP NULL DEFAULT NULL,
    INDEX idx_map_notes_visible (deleted_at, updated_at),
    CONSTRAINT fk_map_notes_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);
