SET
  NAMES utf8mb4;

SET
  FOREIGN_KEY_CHECKS = 0;

DROP DATABASE IF EXISTS AnyoneChat;

CREATE DATABASE AnyoneChat;

USE AnyoneChat;

-- ----------------------------
-- Table structure for chat_message
-- ----------------------------
DROP TABLE IF EXISTS `chat_message`;

CREATE TABLE `chat_message` (
  `message_id` bigint UNSIGNED NOT NULL AUTO_INCREMENT,
  `thread_id` bigint UNSIGNED NOT NULL,
  `sender_id` bigint UNSIGNED NOT NULL,
  `recv_id` bigint UNSIGNED NOT NULL,
  `content` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `status` tinyint NOT NULL DEFAULT 0 COMMENT '0=未读 1=已读 2=撤回',
  PRIMARY KEY (`message_id`) USING BTREE,
  INDEX `idx_thread_created`(`thread_id` ASC, `created_at` ASC) USING BTREE,
  INDEX `idx_thread_message`(`thread_id` ASC, `message_id` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 21 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of chat_message
-- ----------------------------
-- ----------------------------
-- Table structure for chat_thread
-- ----------------------------
DROP TABLE IF EXISTS `chat_thread`;

CREATE TABLE `chat_thread` (
  `id` bigint UNSIGNED NOT NULL AUTO_INCREMENT,
  `type` enum('private', 'group') CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 27 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of chat_thread
-- ----------------------------
-- ----------------------------
-- Table structure for friend
-- ----------------------------
DROP TABLE IF EXISTS `friend`;

CREATE TABLE `friend` (
  `id` int UNSIGNED NOT NULL AUTO_INCREMENT,
  `self_id` int NOT NULL,
  `friend_id` int NOT NULL,
  `back` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT '',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `self_friend`(`self_id` ASC, `friend_id` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 89 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Table structure for friend_apply
-- ----------------------------
DROP TABLE IF EXISTS `friend_apply`;

CREATE TABLE `friend_apply` (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `from_uid` int NOT NULL,
  `to_uid` int NOT NULL,
  `status` smallint NOT NULL DEFAULT 0,
  `descs` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT '',
  `back_name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT '',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `from_to_uid`(`from_uid` ASC, `to_uid` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 68 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Table structure for chat_message
-- ----------------------------
DROP TABLE IF EXISTS `chat_message`;

CREATE TABLE `chat_message` (
  `message_id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `thread_id` BIGINT UNSIGNED NOT NULL,
  `sender_id` BIGINT UNSIGNED NOT NULL,
  `recv_id` BIGINT UNSIGNED NOT NULL,
  `content` TEXT NOT NULL,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `status` TINYINT NOT NULL DEFAULT 0 COMMENT '0=未读 1=已读 2=撤回',
  PRIMARY KEY (`message_id`),
  KEY `idx_thread_created` (`thread_id`, `created_at`),
  KEY `idx_thread_message` (`thread_id`, `message_id`)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COLLATE = utf8mb4_unicode_ci;

-- ----------------------------
-- Records of chat_message
-- ----------------------------
-- ----------------------------
-- Table structure for chat_thread
-- ----------------------------
DROP TABLE IF EXISTS `chat_thread`;

CREATE TABLE chat_thread (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `type` ENUM('private', 'group') NOT NULL,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id)
);

-- ----------------------------
-- Records of chat_thread
-- ----------------------------
-- ----------------------------
-- Table structure for private_chat
-- ----------------------------
DROP TABLE IF EXISTS `private_chat`;

CREATE TABLE `private_chat` (
  `thread_id` BIGINT UNSIGNED NOT NULL COMMENT '引用chat_thread.id',
  `user1_id` BIGINT UNSIGNED NOT NULL,
  `user2_id` BIGINT UNSIGNED NOT NULL,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`thread_id`),
  UNIQUE KEY `uniq_private_thread` (`user1_id`, `user2_id`),
  -- 保证每对用户只能有一个私聊会话
  -- 以下两行就是我们要额外加的复合索引
  KEY `idx_private_user1_thread` (`user1_id`, `thread_id`),
  KEY `idx_private_user2_thread` (`user2_id`, `thread_id`)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4;

-- ----------------------------
-- Records of private_chat
-- ----------------------------
-- ----------------------------
-- Table structure for group_chat
-- ----------------------------
DROP TABLE IF EXISTS `group_chat`;

CREATE TABLE `group_chat` (
  `thread_id` bigint UNSIGNED NOT NULL COMMENT '引用chat_thread.id',
  `name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NULL DEFAULT NULL COMMENT '群聊名称',
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`thread_id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 1 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of group_chat
-- ----------------------------
-- ----------------------------
-- Table structure for group_chat_member
-- ----------------------------
DROP TABLE IF EXISTS `group_chat_member`;

CREATE TABLE `group_chat_member` (
  `thread_id` bigint UNSIGNED NOT NULL COMMENT '引用 group_chat_thread.thread_id',
  `user_id` bigint UNSIGNED NOT NULL COMMENT '引用 user.user_id',
  `role` tinyint NOT NULL DEFAULT 0 COMMENT '0=普通成员,1=管理员,2=创建者',
  `joined_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `muted_until` timestamp NULL DEFAULT NULL COMMENT '如果被禁言，可存到什么时候',
  PRIMARY KEY (`thread_id`, `user_id`) USING BTREE,
  INDEX `idx_user_threads`(`user_id` ASC) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of group_chat_member
-- ----------------------------
-- ----------------------------
-- Table structure for private_chat
-- ----------------------------
DROP TABLE IF EXISTS `private_chat`;

CREATE TABLE `private_chat` (
  `thread_id` bigint UNSIGNED NOT NULL COMMENT '引用chat_thread.id',
  `user1_id` bigint UNSIGNED NOT NULL,
  `user2_id` bigint UNSIGNED NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`thread_id`) USING BTREE,
  UNIQUE INDEX `uniq_private_thread`(`user1_id` ASC, `user2_id` ASC) USING BTREE,
  INDEX `idx_private_user1_thread`(`user1_id` ASC, `thread_id` ASC) USING BTREE,
  INDEX `idx_private_user2_thread`(`user2_id` ASC, `thread_id` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 1 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of private_chat
-- ----------------------------
-- ----------------------------
-- Table structure for user
-- ----------------------------
DROP TABLE IF EXISTS `user`;

CREATE TABLE `user` (
  `id` int NOT NULL AUTO_INCREMENT,
  `uid` int NOT NULL DEFAULT 0,
  `name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `email` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `pwd` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `nick` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `desc` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `sex` int NOT NULL DEFAULT 0,
  `icon` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `uid`(`uid` ASC) USING BTREE,
  UNIQUE INDEX `email`(`email` ASC) USING BTREE,
  INDEX `name`(`name` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 61 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of user
-- ----------------------------
INSERT INTO
  `user`
VALUES
  (
    1,
    1001,
    'rain',
    '20520@126.com',
    '745230',
    'yyh1',
    '',
    0,
    ':/res/head_1.jpg'
  );

INSERT INTO
  `user`
VALUES
  (
    2,
    1002,
    'klaus',
    '285881076@qq.com',
    '745230',
    'yyh2',
    '',
    0,
    ':/res/head_2.jpg'
  );

INSERT INTO
  `user`
VALUES
  (
    3,
    1003,
    'nick',
    '11102766@qq.com',
    '745230',
    'yyh3',
    '',
    0,
    ':/res/head_3.jpg'
  );

-- ----------------------------
-- Table structure for user_id
-- ----------------------------
DROP TABLE IF EXISTS `user_id`;

CREATE TABLE `user_id` (
  `id` int NOT NULL,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of user_id
-- ----------------------------
INSERT INTO
  `user_id`
VALUES
  (1058);

SET
  FOREIGN_KEY_CHECKS = 1;