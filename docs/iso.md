# Прямое чтение ISO / Direct ISO reading

Начиная с alpha4, нажмите **«Выбрать ISO»**, выберите собственный образ DOA3 USA
и дождитесь проверки. Затем нажмите **«ИГРАТЬ»**. ZIP для загрузки игры не поддерживается.

Первая проверка читает все 24 файла и сравнивает размеры и SHA-256. Она не
распаковывает файлы на накопитель. Приложение сохраняет постоянное разрешение
Android на выбранный документ и небольшую таблицу имён, смещений и размеров.
При следующих запусках проверяются доступность, таблица файлов и игровой XBE.

ISO должен оставаться доступным во внутренней памяти или на SD-карте. После
перемещения, удаления или изменения образа выберите его заново. Провайдер должен
поддерживать произвольное чтение. Полный проверенный Redump ISO занимает
7 825 162 240 байт; файловая система FAT32 не подходит для файла такого размера.

Сохранения и кэш самой игры хранятся в приложении. Обновление APK их сохраняет.
Старая извлечённая копия из alpha1–alpha3 автоматически не удаляется; новый режим
читает диск из ISO и её не использует. Сброс данных приложения удаляет сохранения.

## English

From alpha4, use **Select ISO**, choose your own DOA3 USA image, wait for initial
verification, then select **PLAY**. ZIP game loading is unsupported.

Initial verification reads all 24 files and checks sizes and SHA-256 without
extracting resources to storage. Only a persistent Android document grant and a
small extent table are retained. Subsequent launches check availability, the
file table and executable identity. The native disc is read-only and uses bounded
random reads from the image descriptor; each game file has an independent cursor.

Keep the seekable ISO available in internal storage or on an SD card. Reselect
it after moving, deleting or changing it. Saves and game-managed cache remain
inside the app. APK updates retain them. Earlier extracted copies are not
automatically deleted and are not used by this mode.
