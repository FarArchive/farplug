.Language=English,English

plugin.name = NTFS File Information

# File metadata
metadata.col.type = Type
metadata.col.flags = RXCES
metadata.col.data_size = Data Size
metadata.col.disk_size = Disk Size
metadata.col.fragments = Fragments
metadata.col.name_data = Name/Data
metadata.row.unnamed_total = Unnamed DATA Total
metadata.row.named_total = Named DATA Total
metadata.row.nr_total = Non-Resident Total
metadata.file_name = &File:
metadata.multiple = <multiple>
metadata.mft_recs = MFT recs: %u
metadata.hard_links = Hard links: %u
metadata.totals.files = Files: %u
metadata.totals.hl = (+%u hl)
metadata.totals.rp_files = (+%u rp)
metadata.totals.dirs = Directories: %u
metadata.totals.rp_dirs = (+%u rp)
metadata.totals.errors = Errors: %c%c%u%c%c
metadata.totals.excess_fragments = +%S

# Menu
menu.metadata = &Metadata information
menu.content = &Content analysis
menu.panel.on = Open &panel
menu.panel.off = Close &panel
menu.defragment = &Defragment
menu.flat_mode.on = Flat m&ode
menu.flat_mode.off = Normal m&ode
menu.mft_mode.on = MFT &index on
menu.mft_mode.off = MFT &index off

# File panel
file_panel.read_dir.progress.title = Reading directory...
file_panel.read_dir.progress.message = %u objects found
file_panel.read_volume.progress.title = Reading volume...
file_panel.read_volume.progress.message = %u file records found
file_panel.read_volume.progress.console.title = {%u%%} Reading volume...
file_panel.title_prefix = nfi
file_panel.mode.title = Panel mode
file_panel.mode.col_types = Column &types
file_panel.mode.status_col_types = St&atus line column types
file_panel.mode.col_widths = Column &widths
file_panel.mode.status_col_widths = Status l&ine column widths
file_panel.mode.wide = &Fullscreen view
file_panel.show.streams = S&how streams
file_panel.show.main.stream = Show &unnamed DATA streams
file_panel.use.highlighting = Use high&lighting
file_panel.use.usn.journal = Use USN &journal
file_panel.delete.usn.journal = &Delete USN journal after use
file_panel.mode.sort = &Sort by:
file_panel.mode.sort.nothing = Nothing
file_panel.mode.sort.data_size = Data Size
file_panel.mode.sort.disk_size = Disk Size
file_panel.mode.sort.fragments = Fragments
file_panel.mode.sort.streams = Streams
file_panel.mode.sort.hard_links = Hard Links
file_panel.mode.sort.mft_records = MFT Records
file_panel.mode.sort.fragm_level = Fragm. Level
file_panel.mode.sort.valid_size = Valid Size
file_panel.mode.sort.name_len = Path Length
file_panel.mode.col_title.data_size = Data Sz
file_panel.mode.col_title.disk_size = Disk Sz
file_panel.mode.col_title.fragments = Fragm
file_panel.mode.col_title.streams = Stm
file_panel.mode.col_title.hard_links = Lnk
file_panel.mode.col_title.mft_records = MFT
file_panel.mode.col_title.valid_size = ValidSz
file_panel.error_marker = ERROR

# File content analysis settings
content.settings.title = File content analysis
content.settings.compression = &Compression ratio
content.settings.zeros = &Zero clusters
content.settings.crc32 = CRC&32 checksum
content.settings.md5 = MD&5 hash
content.settings.sha1 = SHA&1 hash
content.settings.ed2k = ED&2K hash
content.settings.set_all = &Set all
content.settings.reset_all = &Reset all

# File content analysis results
content.result.title = File content analysis
content.result.processed1 = Processed %S in %S at %S
content.result.processed2 = Processed %S in %S
content.result.compression = &Compression ratio: %S / %S = 
content.result.crc32 = CRC&32 checksum:
content.result.md5 = MD&5 hash:
content.result.sha1 = SHA&1 hash:
content.result.ed2k = ED&2K hash:
content.result.verify = &Verify hash:
content.result.result = Result:
content.result.close = Close
content.result.save_file = &Save to file
content.result.correct = Correct
content.result.wrong = Wrong
content.result.file_exists = File already exists. Overwrite?
content.result.file_saved = File saved successfully.

# File content analysis progress
content.progress.title = Processing
content.progress.processed1 = Processed %8S from %8S [%3u%%] at %10S
content.progress.processed2 = Processed %8S from %8S [%3u%%]
content.progress.elapsed = Elapsed: %8S  Left: %8S  Total: %8S
content.progress.compression = Compression ratio: %8S / %8S = %3u%%
content.progress.console.title = {%u%%} Processing...

# Multiple file content analysis result
content.multi.result.title = File content analysis
content.multi.result.processed = Processed %S in %S
content.multi.result.speed = at %S
content.multi.result.compression = Compression ratio: %S / %S = 
content.multi.result.files = Files processed: %u
content.multi.result.dirs = Directories processed: %u
content.multi.result.reparse = Reparse points skipped: %u
content.multi.result.errors = Errors (objects skipped): %u
content.multi.result.close = Close

# Multiple file content analysis progress
content.multi.progress.title = Processing
content.multi.progress.processed = Processed %S from %S [%u%%]
content.multi.progress.speed = at %S
content.multi.progress.compression = Compression ratio: %S / %S = %Lu%%
content.multi.progress.elapsed = Elapsed: %S  Left: %S  Total: %S
content.multi.progress.files = Files processed: %u from %u
content.multi.progress.dirs = Directories processed: %u from %u
content.multi.progress.reparse = Reparse points skipped: %u
content.multi.progress.errors = Errors (objects skipped): %S
content.multi.progress.console.title = {%u%%} Processing...

# Estimate total size progress
estimate.progress.title = Scanning
estimate.progress.size = Total size: %S
estimate.progress.files = Files: %u
estimate.progress.dirs = Directories: %u
estimate.progress.reparse = Reparse points: %u
estimate.progress.errors = Errors: %S

# Defragmentator
defrag.progress.title = Defragmenting
defrag.progress.file_name = File:
defrag.progress.clusters = Moved %Lu clusters from %Lu total
defrag.progress.extents = Reducing number of extents from %u to %u
defrag.progress.analyze = Analyzing volume
defrag.progress.files = Processed %u files from %u total
defrag.progress.console.title = {%Lu%%} Defragmenting...
defrag.errors = Some files were not defragmented because of errors. See log for details.

# Error log
log.title = Error Log
log.defragment = Defragment
log.show = &Show error log

# Generic buttons
button.ok = OK
button.cancel = Cancel

# Size and speed suffixes
suffix.size.b = B
suffix.size.kb = KB
suffix.size.mb = MB
suffix.size.gb = GB
suffix.size.tb = TB
suffix.speed.b = B/s
suffix.speed.kb = KB/s
suffix.speed.mb = MB/s
suffix.speed.gb = GB/s
suffix.speed.tb = TB/s
suffix.alt.size.kb = KiB
suffix.alt.size.mb = MiB
suffix.alt.size.gb = GiB
suffix.alt.size.tb = TiB
suffix.alt.speed.kb = KiB/s
suffix.alt.speed.mb = MiB/s
suffix.alt.speed.gb = GiB/s
suffix.alt.speed.tb = TiB/s
suffix.short.size.kb = K
suffix.short.size.mb = M
suffix.short.size.gb = G
suffix.short.size.tb = T
