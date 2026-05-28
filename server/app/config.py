from pydantic_settings import BaseSettings


class Settings(BaseSettings):
    database_url: str
    secret_key: str
    algorithm: str = "HS256"
    access_token_expire_minutes: int = 10080  # 7 days

    firebase_credentials_path: str
    tesseract_cmd: str = "/usr/bin/tesseract"

    class Config:
        env_file = ".env"


settings = Settings()
