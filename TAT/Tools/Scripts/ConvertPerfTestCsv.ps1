# (c) 2025 OtherSide Entertainment, Inc
# SPDX-License-Identifier: MIT
# converts perftest CSV into format usable by plot jenkins plugin
$in_folder = $Env:PERF_TEST_FOLDER
$out_folder = $Env:DERIVED_PERF_TEST_FOLDER
# find last file with changelist and strip out the header
$filename = Get-ChildItem $in_folder\perf_${Env:MAP_NAME}_${Env:P4_CHANGELIST}_*.csv | Sort-Object LastWriteTime -Descending |  Select-Object -First 1
Get-Content $filename | Select-Object -Skip 16 | Out-File -FilePath $out_folder\temp.csv
$csv = Import-Csv -Path $out_folder\temp.csv

$levels = (0, 'low'), (1, 'medium'), (2, 'high'), (3, 'veryhigh')
$stats = @(('avg_frame', 'frame'), ('avg_render_thread', 'render_thread'), ('avg_game_thread', 'game_thread'), ('avg_gpu_0_frame', 'gpu'), ('avg_render_thread_critical_path', 'render_thread_criticalpath'), ('avg_rhi', 'rhi'))

$per_shot_stats = @(('avg_frame', 'shots'), ('avg_prims_drawn', 'shot_prims'), ('avg_draw_calls', 'shot_draws'), ('avg_gpu_0_frame', 'shot_gpu'), ('avg_game_thread', 'shot_gamethread'), ('avg_render_thread', 'shot_renderthread'), ('avg_render_thread_critical_path', 'shot_renderthreadcrit'), ('max_gpu_frame_0', 'shot_gpumax'), ('max_frame', 'shot_maxframe'), ('max_ai', 'shot_ai'), ('max_ai_rendered', 'shot_ai_rendered'), ('avg_rhi', 'shot_rhi'))

# collect stats by scalability level
foreach($level_pair in $levels)
{
	$index = $level_pair[0]
	$level_name = $level_pair[1]

	$for_level = $csv |  Where-Object {$_.scalability -eq $index}
	$plot_header = @()
	$plot_value = @()

	$bench_lines = @("MetricName,Value")

	foreach($stat in $stats)
	{
		$stat_display = $stat[1]
		$derived = $for_level | Measure-Object -Property $stat[0] -Minimum -Maximum -Average
		$plot_header += @("${stat_display}_mean", "${stat_display}_min", "${stat_display}_max")
		$plot_value += @($derived.Average, $derived.Minimum, $derived.Maximum)

		$bench_lines += "${stat_display}_mean,$($derived.Average)"
	}

	@(($plot_header -join ','), ($plot_value -join ',')) | Out-File -encoding ASCII -FilePath $out_folder\plot_${level_name}.csv
	$bench_lines | Out-File -encoding ASCII -FilePath $out_folder\bench_${level_name}.csv

	foreach($shot_stat in $per_shot_stats)
	{
		$file_suffix = $shot_stat[1]
		$shot_names = ($for_level | % {"$($_.name)"}) -join ','
		$shot_values = ($for_level | % {"$($_.$($shot_stat[0]))"}) -join ','
		($shot_names, $shot_values) | Out-File -encoding ASCII -FilePath $out_folder\plot_${file_suffix}_${level_name}.csv
	}
}
