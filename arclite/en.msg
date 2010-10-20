.Language=English,English

plugin.name = <(NAME)>

# Extract dialog
extract.dlg.title = Extract files
extract.dlg.dst_dir = &Extract files to:
extract.dlg.ignore_errors = &Ignore errors
extract.dlg.oo = When destination file exists:
extract.dlg.oo_ask = &Ask
extract.dlg.oo_overwrite = &Overwrite
extract.dlg.oo_skip = &Skip
extract.dlg.move_files = &Delete files from archive after extraction
extract.dlg.password = Pass&word:
extract.dlg.separate_dir = Extract archive into separate di&rectory

# Update dialog
update.dlg.title = Add files to archive
update.dlg.title.create = Create archive
update.dlg.arc_path = &Archive path:
update.dlg.arc_type = Select archiver:
update.dlg.arc_type.other = &0ther:
update.dlg.level = Compression level:
update.dlg.method = Compression method:
update.dlg.solid = S&olid archive
update.dlg.encrypt = &Encrypt archive
update.dlg.show_password = Show password
update.dlg.password = Pass&word:
update.dlg.password2 = Repeat:
update.dlg.encrypt_header = Encrypt &header
update.dlg.create_sfx = Create self-e&xtracting archive using SFX module:
update.dlg.enable_volumes = Split into &volumes
update.dlg.volume_size = Volume size:
update.dlg.move_files = &Delete files after archiving
update.dlg.open_shared = Archive files open for w&riting
update.dlg.ignore_errors = &Ignore errors
update.dlg.wrong_arc_type = Select archive format
update.dlg.wrong_level = Select compression level
update.dlg.wrong_method = Select compression method
update.dlg.passwords_dont_match = Passwords don't match
update.dlg.password_is_empty = Password is empty
update.dlg.wrong_sfx_module = Select SFX module
update.dlg.wrong_volume_size = Invalid volume size
update.dlg.confirm_overwrite = Target file already exists. Do you want to overwrite it?

# Archive types
compression.archive.7z = &7z
compression.archive.zip = &zip

# Compression levels
compression.level.store = &Store
compression.level.fastest = Fas&test
compression.level.fast = &Fast
compression.level.normal = &Normal
compression.level.maximum = &Maximum
compression.level.ultra = &Ultra

# Compression methods
compression.method.lzma = &LZMA
compression.method.lzma2 = LZMA&2
compression.method.ppmd = &PPMD

# Delete files
delete.dlg.confirm = Do you want to delete selected files?

# Test archive
test.ok = Archive test complete
test.menu = Test archive files

# Password dialog
password.title = Enter password
password.password = Password:

# Overwrite dialog
overwrite.dlg.title = Overwrite file
overwrite.dlg.question = Destination file already exists. Overwrite?
overwrite.dlg.source = Source:
overwrite.dlg.destination = Destination:
overwrite.dlg.newer = (newer)
overwrite.dlg.yes = &Yes
overwrite.dlg.yes.all = Yes to &All
overwrite.dlg.no = &No
overwrite.dlg.no.all = No to A&ll

# Error retry dialog
button.retry = &Retry
button.ignore = &Ignore
button.ignore_all = Ignore &All

# Log dialog
log.title = Operation log
log.info = Operation completed with errors. Do you want to review log file?
log.close = &Close
log.show = Show &log

# Settings dialog
settings.dlg.handle_create = Handle archive creation (Shift-F&1)
settings.dlg.handle_commands = Handle archive commands (Ctrl-PgDn, Shift-F&2)
settings.dlg.use_include_masks = &Open files matching masks:
settings.dlg.use_exclude_masks = &Exclude files matching masks:
settings.dlg.generate_masks = &Generate masks

# Attributes dialog
attr.dlg.title = Attributes

# Progress messages
progress.open = Reading archive...
progress.extract = Extracting files...
progress.create_dirs = Creating directories...
progress.set_attr = Setting attributes...
progress.test = Testing archive...
progress.update = Updating archive...
progress.create = Creating archive...
progress.delete_files = Deleting files...
progress.scan_dirs = Scanning directories...
progress.interrupt = Do you want to interrupt current operation?
progress.pause = P = pause
progress.unpause = paused (P = continue)
progress.low_priority = B = low priority
progress.normal_priority = low priority (B = normal)
progress.paused = Paused

# Error messages
error.extract.unsupported_method = Unsupported compression method
error.extract.data_error = Data error
error.extract.data_error.encrypted = Wrong password or data error
error.extract.crc_error = CRC check failed
error.extract.crc_error.encrypted = Wrong password or CRC check failure
error.update.error = Update error
error.no_format = Archive format not found
error.not_updatable = Archive modification not supported

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

# Property names
kpid.Path = Path
kpid.Name = Name
kpid.Extension = Extension
kpid.IsDir = Is Folder
kpid.Size = Size
kpid.PackSize = Packed Size
kpid.Attrib = Attributes
kpid.CTime = Created
kpid.ATime = Accessed
kpid.MTime = Modified
kpid.Solid = Solid
kpid.Commented = Commented
kpid.Encrypted = Encrypted
kpid.SplitBefore = Splited Before
kpid.SplitAfter = Splited After
kpid.DictionarySize = Dictionary Size
kpid.CRC = CRC
kpid.Type = Type
kpid.IsAnti = Anti
kpid.Method = Method
kpid.HostOS = Host OS
kpid.FileSystem = File System
kpid.User = User
kpid.Group = Group
kpid.Block = Block
kpid.Comment = Comment
kpid.Position = Position
kpid.Prefix = Prefix
kpid.NumSubDirs = Folders
kpid.NumSubFiles = Files
kpid.UnpackVer = Version
kpid.Volume = Volume
kpid.IsVolume = Multivolume
kpid.Offset = Offset
kpid.Links = Links
kpid.NumBlocks = Blocks
kpid.NumVolumes = Volumes
kpid.TimeType = Time Type
kpid.Bit64 = 64-bit
kpid.BigEndian = Big-endian
kpid.Cpu = CPU
kpid.PhySize = Physical Size
kpid.HeadersSize = Headers Size
kpid.Checksum = Checksum
kpid.Characts = Characteristics
kpid.Va = Virtual Address
kpid.Id = ID
kpid.ShortName = Short Name
kpid.CreatorApp = Creator Application
kpid.SectorSize = Sector Size
kpid.PosixAttrib = Mode
kpid.Link = Link
kpid.TotalSize = Total Size
kpid.FreeSpace = Free Space
kpid.ClusterSize = Cluster Size
kpid.VolumeName = Label
kpid.LocalName = Local Name
kpid.Provider = Provider

# Property values
property.true = Yes
property.false = No
property.compression_ratio = Compression ratio
