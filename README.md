# dir-watch

## Установка

```bash
sudo dpkg -i dir-watch_1.0.0_amd64.deb
chmod +x install.sh
sudo ./install.sh
```

## Использование

```bash
dir-watch -s /path/to/dir          # создать .snapshot
dir-watch -c /path/to/dir          # сравнить с .snapshot
dir-watch -e report.txt            # сохранить последний отчет
dir-watch -h                       # справка
```

Снимок хранится в `.snapshot` внутри проверяемого каталога. Последний отчет
сравнения временно хранится в `/tmp/.dir-watch-changes-{user_id}`.
