m4_include(`farversion.m4')m4_dnl
version=MAJOR.MINOR.BUILD

!if "$(CPU)"=="AMD64" 
suffix=64
platform=x64
distrsuffix=_x64
!else
suffix=32
platform=x86
!endif

all:
!ifdef bundle
  cl -nologo -O1 -EHsc -Dbundle installer\custom.cpp -link -dll -out:CustomActions.dll -export:UpdateFeatureState -export:Optimize msi.lib imagehlp.lib
  candle -nologo -Iinstaller -dSuffix=$(suffix) -dPlatform=$(platform) -dVersion=$(version) -dbundle=$(bundle) installer\installer.wxs installer\MyWixUI_FeatureTree.wxs
  light -nologo -ext WixUIExtension -cultures:en-us -spdb -sval -cc . -reusecab -dcl:high -dWixUIBannerBmp=installer\bundle-banner.bmp -dWixUIDialogBmp=installer\bundle-dialog.bmp -out far_$(version)$(distrsuffix)_bundle.msi installer.wixobj MyWixUI_FeatureTree.wixobj
!else
  cl -nologo -O1 -EHsc installer\custom.cpp -link -dll -out:CustomActions.dll -export:UpdateFeatureState -export:Optimize msi.lib imagehlp.lib
  candle -nologo -Iinstaller -dSuffix=$(suffix) -dPlatform=$(platform) -dVersion=$(version) installer\installer.wxs installer\MyWixUI_FeatureTree.wxs
  light -nologo -ext WixUIExtension -cultures:en-us -spdb -sval -dcl:high -dWixUIBannerBmp=installer\banner.bmp -dWixUIDialogBmp=installer\dialog.bmp -out far_$(version)_uni$(distrsuffix)_ntfsfile.msi installer.wixobj MyWixUI_FeatureTree.wixobj
!endif
