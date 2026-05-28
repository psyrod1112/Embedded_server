"""APScheduler jobs — expiry alert at 09:00 and 15:00 daily."""

import logging
from datetime import date, timedelta

from apscheduler.schedulers.background import BackgroundScheduler
from apscheduler.triggers.cron import CronTrigger
from sqlalchemy import func
from sqlalchemy.orm import Session

from app.database import SessionLocal
from app.models import FoodItem, FoodType, User
from app.services import fcm as fcm_service

logger = logging.getLogger(__name__)
scheduler = BackgroundScheduler(timezone="Asia/Seoul")


def _check_expiry():
    db: Session = SessionLocal()
    try:
        threshold = date.today() + timedelta(days=3)

        expiring_count = (
            db.query(func.sum(FoodItem.quantity))
            .join(FoodType, FoodItem.type_id == FoodType.type_id)
            .filter(
                FoodItem.expiry_date <= threshold,
                FoodItem.deleted_at.is_(None),
                FoodType.deleted_at.is_(None),
            )
            .scalar()
        )

        if not expiring_count:
            return

        users = db.query(User).filter(User.fcm_token.isnot(None)).all()
        for user in users:
            fcm_service.send_expiry_alert(user.fcm_token, int(expiring_count))
            logger.info("Expiry alert sent to %s (%d items)", user.username, expiring_count)
    except Exception as e:
        logger.error("Expiry check failed: %s", e)
    finally:
        db.close()


def start():
    scheduler.add_job(_check_expiry, CronTrigger(hour=9, minute=0))
    scheduler.add_job(_check_expiry, CronTrigger(hour=15, minute=0))
    scheduler.start()
    logger.info("Scheduler started")


def stop():
    scheduler.shutdown()
