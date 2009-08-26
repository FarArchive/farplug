m4_include(`farversion.m4')m4_dnl
version=MAJOR.MINOR.BUILD

msi_name=far_$(version)

!if "$(CPU)"=="AMD64" 
platform=x64
msi_name=$(msi_name)_x64
!else
platform=x86
!endif

!ifdef bundle
msi_name=$(msi_name)_bundle
!endif

all:
!ifdef bundle
  cl -nologo -O1 -EHsc -Dbundle installer\custom.cpp -link -dll -out:CustomActions.dll -export:UpdateFeatureState -export:Optimize msi.lib imagehlp.lib
  candle -nologo -Iinstaller -dPlatform=$(platform) -dVersion=$(version) -dbundle=$(bundle) installer\installer.wxs installer\MyWixUI_FeatureTree.wxs
  light -nologo -ext WixUIExtension -cultures:en-us -spdb -sval -cc . -reusecab -dcl:high -dWixUIBannerBmp=installer\bundle-banner.bmp -dWixUIDialogBmp=installer\bundle-dialog.bmp -out $(msi_name).msi installer.wixobj MyWixUI_FeatureTree.wixobj
!else
  cl -nologo -O1 -EHsc installer\custom.cpp -link -dll -out:CustomActions.dll -export:UpdateFeatureState -export:Optimize msi.lib imagehlp.lib
  candle -nologo -Iinstaller -dPlatform=$(platform) -dVersion=$(version) installer\installer.wxs installer\MyWixUI_FeatureTree.wxs
  light -nologo -ext WixUIExtension -cultures:en-us -spdb -sval -dcl:high -dWixUIBannerBmp=installer\banner.bmp -dWixUIDialogBmp=installer\dialog.bmp -out $(msi_name).msi installer.wixobj MyWixUI_FeatureTree.wixobj
!endif
