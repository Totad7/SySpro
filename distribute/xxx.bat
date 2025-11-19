cat > distribute/Run.bat << 'EOF'
@echo off
chcp 65001 >nul
pc_diagnostic.exe
pause
EOF

echo "Статически скомпилированная версия с OpenCL готова!"