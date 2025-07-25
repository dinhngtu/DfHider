@echo off
codeql database create -l=cpp -s=DFHider -c "msbuild /t:Rebuild /p:Configuration=Release /p:Platform=x64" database --overwrite
codeql database analyze database microsoft/windows-drivers:windows-driver-suites/recommended.qls --output=DFHider.sarif --format=sarifv2.1.0
