#!/usr/bin/env zsh

# 预览 Oh-My-Zsh 主题（shell 中无法实时预览，失败脚本）

# 1. 先保存当前主题
current_theme=$(grep 'ZSH_THEME=' ~/.zshrc | cut -d '"' -f 2)

# 2. 遍历所有主题，逐个预览
for theme in ~/.oh-my-zsh/themes/*; do
	theme_name=$(basename $theme .zsh-theme)
	perl -i -pe "s/ZSH_THEME=\".*\"/ZSH_THEME=\"$theme_name\"/" ~/.zshrc
	source ~/.zshrc
	echo "当前主题：$theme_name"
	sleep 1
done

# 3. 恢复当前主题
perl -i -pe "s/ZSH_THEME=\".*\"/ZSH_THEME=\"$current_theme\"/" ~/.zshrc
source ~/.zshrc
echo "已恢复到原主题：$current_theme"
