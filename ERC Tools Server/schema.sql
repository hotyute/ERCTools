CREATE DATABASE IF NOT EXISTS erc_tools CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE erc_tools;

CREATE TABLE IF NOT EXISTS users (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(128) NOT NULL UNIQUE,
    display_name VARCHAR(255) NOT NULL,
    position ENUM('Administrator', 'Supervisor', 'Manager', 'ERC') NOT NULL,
    pod ENUM('Pod 1', 'Pod 2', 'Pod 3', 'Pod 4', 'Pod 5', 'Pod 6', 'Pod 7', 'Pod 8', 'Pod 9', 'Pod 10') NOT NULL,
    password_salt CHAR(32) NOT NULL,
    password_hash CHAR(64) NOT NULL,
    password_iterations INT NOT NULL DEFAULT 150000,
    active TINYINT(1) NOT NULL DEFAULT 1,
    muted_until TIMESTAMP NULL DEFAULT NULL,
    muted_by BIGINT UNSIGNED NULL DEFAULT NULL,
    muted_at TIMESTAMP NULL DEFAULT NULL,
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
    deleted_at TIMESTAMP NULL DEFAULT NULL,
    deleted_by BIGINT UNSIGNED NULL DEFAULT NULL,
    INDEX idx_chat_messages_created (created_at),
    CONSTRAINT fk_chat_messages_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS private_messages (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    sender_user_id BIGINT UNSIGNED NOT NULL,
    recipient_user_id BIGINT UNSIGNED NOT NULL,
    body TEXT NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    deleted_by_sender_at TIMESTAMP NULL DEFAULT NULL,
    deleted_by_recipient_at TIMESTAMP NULL DEFAULT NULL,
    INDEX idx_private_messages_pair (sender_user_id, recipient_user_id, created_at),
    INDEX idx_private_messages_recipient (recipient_user_id, created_at),
    CONSTRAINT fk_private_messages_sender FOREIGN KEY (sender_user_id) REFERENCES users(id) ON DELETE CASCADE,
    CONSTRAINT fk_private_messages_recipient FOREIGN KEY (recipient_user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS user_session_audit (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    event_type VARCHAR(32) NOT NULL,
    target_user_id BIGINT UNSIGNED NULL DEFAULT NULL,
    username VARCHAR(128) NULL DEFAULT NULL,
    display_name VARCHAR(255) NULL DEFAULT NULL,
    position VARCHAR(32) NULL DEFAULT NULL,
    pod VARCHAR(32) NULL DEFAULT NULL,
    actor_user_id BIGINT UNSIGNED NULL DEFAULT NULL,
    actor_username VARCHAR(128) NULL DEFAULT NULL,
    details VARCHAR(255) NULL DEFAULT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_user_session_audit_created (created_at),
    INDEX idx_user_session_audit_user (target_user_id)
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

CREATE TABLE IF NOT EXISTS incident_exclusions (
    key_hash CHAR(64) NOT NULL PRIMARY KEY,
    incident_key VARCHAR(1024) NOT NULL,
    source_id VARCHAR(255) NULL DEFAULT NULL,
    source_name VARCHAR(64) NULL DEFAULT NULL,
    road VARCHAR(64) NULL DEFAULT NULL,
    summary VARCHAR(512) NULL DEFAULT NULL,
    added_by_user_id BIGINT UNSIGNED NULL DEFAULT NULL,
    added_by_username VARCHAR(128) NULL DEFAULT NULL,
    added_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_incident_exclusions_added (added_at),
    INDEX idx_incident_exclusions_source (source_id)
);
