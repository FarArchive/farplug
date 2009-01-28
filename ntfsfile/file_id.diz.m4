m4_include(`version.m4')m4_dnl
NTFS File Information __VER_MAJOR__.__VER_MINOR__.__VER_PATCH__
FAR Manager Plugin

Плагин предназначен для получения различной информации о файлах на файловой системе NTFS. На данный момент реализованы функции:

1. Определение занимаемого файлом места на диске. Особенно полезно для сжатых и разрежённых файлов. Для каталогов также определяется суммарный объем с учётом содержимого.
2. Отображение списка NTFS атрибутов, из которых состоит файл. Можно использовать для просмотра:
  - альтернативных имён файла (жёстких связей).
  - альтернативных потоков данных (именованных атрибутов DATA).
  - целевого каталога / имени тома для символических связей / точек монтирования.
3. Определение степени фрагментации файлов. Дефрагментация файлов и каталогов.
4. Функции анализа данных:
  - оценка сжимаемости данных для группы файлов/каталогов (производится сжатие данных с помощью быстрого алгоритма LZO, полезно чтобы определить - стоит ли сжимать файлы средствами NTFS или архиватором).
  - расчёт наиболее полезных хешей для выбранного файла: crc32, md5, sha1, ed2k (вариант eMule).

Системные требования:
  - Far m4_ifdef(`__FARAPI17__', `1.71.2411', `2.0.680')
  - Windows 2000+

Новые версии: http://forum.farmanager.com/viewtopic.php?t=2050
Обсуждение: http://forum.farmanager.com/viewtopic.php?t=2051

---------------------------------------------------------------

This plugin provides various information about files on NTFS file system. Features:

1. Determine file size on disk. Especially useful for compressed and sparse files. Total size is shown for multiple files / directories.
2. List NTFS attributes (file components on NTFS file system). Can be used to get information about:
  - alternate file names (hard links)
  - alternate data streams (named DATA attribute)
  - symbolic link targets
3. Analyze file fragmentation and perform defragmentation.
4. Analyze file contents:
  - estimate if compression of file data is possible (using very FAST LZO algorithm)
  - calculate most useful file hashes: crc32, md5, sha1, ed2k (eMule variation).

System requirements:
  - Far m4_ifdef(`__FARAPI17__', `1.71.2411', `2.0.680')
  - Windows 2000+

New versions: http://forum.farmanager.com/viewtopic.php?t=2050
Discussion: http://forum.farmanager.com/viewtopic.php?t=2051
