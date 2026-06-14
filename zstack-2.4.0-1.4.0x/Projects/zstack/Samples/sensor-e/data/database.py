"""
database.py — 数据存储 (SQLite/MySQL 双支持)
E同学：Python应用层负责人
实现 access_log / door_env 两张表的 CRUD 操作
支持 SQLite 与 MySQL 灵活切换，通过 config.DATABASE_TYPE 控制
"""

import os
import logging
from datetime import datetime, timedelta
from contextlib import contextmanager

import config
from data.models import AccessLog, DoorEnv

logger = logging.getLogger(__name__)


class Database:
    """本地/远程 数据库管理"""

    def __init__(self):
        self.db_type = config.DATABASE_TYPE.lower()
        if self.db_type == "mysql":
            import pymysql
            self._pymysql = pymysql
        else:
            import sqlite3
            self._sqlite3 = sqlite3
            self.db_path = config.SQLITE_DB_PATH
        
        self._init_database_if_mysql()
        self._init_tables()

    def _init_database_if_mysql(self):
        """如果是MySQL，确保数据库存在"""
        if self.db_type == "mysql":
            try:
                conn = self._pymysql.connect(
                    host=config.MYSQL_HOST,
                    port=config.MYSQL_PORT,
                    user=config.MYSQL_USER,
                    password=config.MYSQL_PASSWORD,
                    charset='utf8mb4',
                    ssl_disabled=True
                )
                with conn.cursor() as cursor:
                    cursor.execute(f"CREATE DATABASE IF NOT EXISTS {config.MYSQL_DATABASE}")
                conn.commit()
                conn.close()
            except Exception as e:
                logger.error("MySQL 创建数据库失败: %s", e)

    @contextmanager
    def _get_conn(self):
        """获取数据库连接（上下文管理器，自动提交/关闭）"""
        if self.db_type == "mysql":
            conn = self._pymysql.connect(
                host=config.MYSQL_HOST,
                port=config.MYSQL_PORT,
                user=config.MYSQL_USER,
                password=config.MYSQL_PASSWORD,
                database=config.MYSQL_DATABASE,
                charset='utf8mb4',
                cursorclass=self._pymysql.cursors.DictCursor,
                ssl_disabled=True
            )
        else:
            conn = self._sqlite3.connect(self.db_path)
            conn.row_factory = self._sqlite3.Row
            
        try:
            yield conn
            conn.commit()
        except Exception:
            conn.rollback()
            raise
        finally:
            conn.close()

    def _init_tables(self):
        """初始化数据库表结构"""
        with self._get_conn() as conn:
            cursor = conn.cursor()
            
            auto_inc = "AUTO_INCREMENT" if self.db_type == "mysql" else "AUTOINCREMENT"
            
            # access_log 表
            cursor.execute(f"""
                CREATE TABLE IF NOT EXISTS access_log (
                    id INTEGER PRIMARY KEY {auto_inc},
                    student_id VARCHAR(50) DEFAULT '',
                    sensor VARCHAR(50) DEFAULT '',
                    event_type VARCHAR(50) DEFAULT '',
                    value INTEGER DEFAULT 0,
                    alert INTEGER DEFAULT 0,
                    is_remote_unlock INTEGER DEFAULT 0,
                    create_time DATETIME DEFAULT CURRENT_TIMESTAMP
                )
            """)

            # door_env 表
            cursor.execute(f"""
                CREATE TABLE IF NOT EXISTS door_env (
                    id INTEGER PRIMARY KEY {auto_inc},
                    student_id VARCHAR(50) DEFAULT '',
                    temp REAL DEFAULT 0.0,
                    humi REAL DEFAULT 0.0,
                    lux INTEGER DEFAULT 0,
                    create_time DATETIME DEFAULT CURRENT_TIMESTAMP
                )
            """)

    def _param(self):
        """返回参数占位符"""
        return "%s" if self.db_type == "mysql" else "?"

    # ======================== access_log CRUD ========================

    def insert_access_log(self, log: AccessLog):
        """INSERT — 插入一条访客事件"""
        p = self._param()
        with self._get_conn() as conn:
            cursor = conn.cursor()
            cursor.execute(
                f"""INSERT INTO access_log
                   (student_id, sensor, event_type, value, alert, is_remote_unlock, create_time)
                   VALUES ({p}, {p}, {p}, {p}, {p}, {p}, {p})""",
                (log.student_id, log.sensor, log.event_type,
                 log.value, log.alert, int(log.is_remote_unlock),
                 log.create_time.strftime("%Y-%m-%d %H:%M:%S"))
            )
            return cursor.lastrowid

    def select_access_logs(self, student_id="", limit=100):
        """SELECT — 查询最近的访客事件（按时间倒序）"""
        p = self._param()
        with self._get_conn() as conn:
            cursor = conn.cursor()
            if student_id:
                cursor.execute(
                    f"SELECT * FROM access_log WHERE student_id = {p} ORDER BY create_time DESC LIMIT {p}",
                    (student_id, limit)
                )
            else:
                if self.db_type == "mysql":
                    cursor.execute(f"SELECT * FROM access_log ORDER BY create_time DESC LIMIT {limit}")
                else:
                    cursor.execute(f"SELECT * FROM access_log ORDER BY create_time DESC LIMIT {p}", (limit,))
            rows = cursor.fetchall()
            return [self._row_to_access_log(r) for r in rows]

    def update_access_log_unlock(self, log_id: int):
        """UPDATE — 标记某条记录为远程开门"""
        p = self._param()
        with self._get_conn() as conn:
            cursor = conn.cursor()
            cursor.execute(f"UPDATE access_log SET is_remote_unlock = 1 WHERE id = {p}", (log_id,))

    def delete_old_access_logs(self, days=30):
        """DELETE — 删除超过指定天数的旧记录"""
        p = self._param()
        cutoff = datetime.now() - timedelta(days=days)
        with self._get_conn() as conn:
            cursor = conn.cursor()
            cursor.execute(
                f"DELETE FROM access_log WHERE create_time < {p}",
                (cutoff.strftime("%Y-%m-%d %H:%M:%S"),)
            )
            return cursor.rowcount

    # ======================== door_env CRUD ========================

    def insert_door_env(self, env: DoorEnv):
        """INSERT — 插入一条环境数据"""
        p = self._param()
        with self._get_conn() as conn:
            cursor = conn.cursor()
            cursor.execute(
                f"""INSERT INTO door_env
                   (student_id, temp, humi, lux, create_time)
                   VALUES ({p}, {p}, {p}, {p}, {p})""",
                (env.student_id, env.temp, env.humi, env.lux,
                 env.create_time.strftime("%Y-%m-%d %H:%M:%S"))
            )
            return cursor.lastrowid

    def select_door_env(self, student_id="", limit=100):
        """SELECT — 查询最近的环境数据"""
        p = self._param()
        with self._get_conn() as conn:
            cursor = conn.cursor()
            if student_id:
                cursor.execute(
                    f"SELECT * FROM door_env WHERE student_id = {p} ORDER BY create_time DESC LIMIT {p}",
                    (student_id, limit)
                )
            else:
                if self.db_type == "mysql":
                    cursor.execute(f"SELECT * FROM door_env ORDER BY create_time DESC LIMIT {limit}")
                else:
                    cursor.execute(f"SELECT * FROM door_env ORDER BY create_time DESC LIMIT {p}", (limit,))
            rows = cursor.fetchall()
            return [self._row_to_door_env(r) for r in rows]

    # ======================== 统计查询 ========================

    def get_daily_visitor_count(self, days=7):
        """获取最近 N 天的每日访客次数"""
        p = self._param()
        cutoff = datetime.now() - timedelta(days=days)
        with self._get_conn() as conn:
            cursor = conn.cursor()
            cursor.execute(
                f"""SELECT DATE(create_time) AS day, COUNT(*) AS cnt
                   FROM access_log
                   WHERE create_time >= {p}
                   GROUP BY DATE(create_time)
                   ORDER BY day ASC""",
                (cutoff.strftime("%Y-%m-%d %H:%M:%S"),)
            )
            return [(str(row["day"]), row["cnt"]) for row in cursor.fetchall()]

    def get_event_type_stats(self, days=7):
        """获取最近 N 天各事件类型的数量统计"""
        p = self._param()
        cutoff = datetime.now() - timedelta(days=days)
        with self._get_conn() as conn:
            cursor = conn.cursor()
            cursor.execute(
                f"""SELECT event_type, COUNT(*) AS cnt
                   FROM access_log
                   WHERE create_time >= {p}
                   GROUP BY event_type
                   ORDER BY cnt DESC""",
                (cutoff.strftime("%Y-%m-%d %H:%M:%S"),)
            )
            return [(row["event_type"], row["cnt"]) for row in cursor.fetchall()]

    # ======================== 辅助方法 ========================

    @staticmethod
    def _row_to_access_log(row) -> AccessLog:
        """将数据库行转换为 AccessLog 对象"""
        create_time = row["create_time"]
        if isinstance(create_time, str):
            create_time = datetime.strptime(create_time, "%Y-%m-%d %H:%M:%S")
        return AccessLog(
            id=row["id"],
            student_id=row["student_id"],
            sensor=row["sensor"],
            event_type=row["event_type"],
            value=row["value"],
            alert=row["alert"],
            is_remote_unlock=bool(row["is_remote_unlock"]),
            create_time=create_time
        )

    @staticmethod
    def _row_to_door_env(row) -> DoorEnv:
        """将数据库行转换为 DoorEnv 对象"""
        create_time = row["create_time"]
        if isinstance(create_time, str):
            create_time = datetime.strptime(create_time, "%Y-%m-%d %H:%M:%S")
        return DoorEnv(
            id=row["id"],
            student_id=row["student_id"],
            temp=row["temp"],
            humi=row["humi"],
            lux=row["lux"],
            create_time=create_time
        )
