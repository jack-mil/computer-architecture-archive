import re
from collections import defaultdict
import subprocess
import sys

isUnderGrad = len(sys.argv) == 2 and sys.argv[1] == "-u"

referenceRe = re.compile(r".*\/results\/([A-B][1-4])\.(.*)\.res\:(\S*)\s*\:\s*(\S*)\s*", re.M)
cpiRe = re.compile(r"LAB2_CPI\s*:\s*(\S*)\s*", re.M)
mispredRe = re.compile(r"LAB2_MISPRED_RATE\s*:\s*(\S*)\s*", re.M)

referenceScores = defaultdict(lambda: defaultdict(lambda: {}))   # {section: {trace: {metric: score}}}
traces = set()

with open('../reference/reference_output.txt', 'r') as f:
  report = f.read()
  for match in referenceRe.finditer(report):
    section, trace, metric, score = match.groups()
    referenceScores[section][trace][metric] = float(score)
    traces.add(trace)
  traces = sorted(list(traces))
traceWeightage = 1 / len(traces)

def run_A(section, command_args):
  print(f"<---------------------- RUNNING {section} ---------------------->")
  try:
    score = 0
    for trace in traces:
      print(f"Testing trace {trace}...")
      actual_cpi = referenceScores[section][trace]["LAB2_CPI"]

      sim = subprocess.Popen(["../src/sim", *command_args, f"../traces/{trace}.ptr.gz"], stdout=subprocess.PIPE)
      res = sim.communicate(timeout=10)[0].decode("utf-8")
      student_cpi = float(cpiRe.search(res).group(1))
      print("LAB2_CPI:", student_cpi)
      
      if actual_cpi * 0.99 <= student_cpi <= actual_cpi * 1.01:
        print("Correct!")
        score += traceWeightage
      elif actual_cpi * 0.95 <= student_cpi <= actual_cpi * 1.05:
        print("Partially correct.")
        score += traceWeightage / 2
      else:
        print("Incorrect.")
      print()
        
    return score
  except subprocess.TimeoutExpired:
    sim.kill()
    exit()
   
def run_B(section, command_args):
  print(f"<---------------------- RUNNING {section} ---------------------->")
  try:
    score = 0
    for trace in traces:
      print(f"Testing trace {trace}...")
      actual_cpi = referenceScores[section][trace]["LAB2_CPI"]
      actual_mispred = referenceScores[section][trace]["LAB2_MISPRED_RATE"]
      
      sim = subprocess.Popen(["../src/sim", *command_args, f"../traces/{trace}.ptr.gz"], stdout=subprocess.PIPE)
      res = sim.communicate(timeout=10)[0].decode("utf-8")

      student_cpi = float(cpiRe.search(res).group(1))
      student_mispred = mispredRe.search(res).group(1)
      print("LAB2_CPI:", student_cpi)
      print("LAB2_MISPRED_RATE:", student_mispred)

      if student_mispred.lower().find("nan") == -1:
        student_mispred = float(student_mispred)
      else:
        print("Branch Prediction not implemented. Exiting...\n")
        break
      
      if actual_cpi * 0.99 <= student_cpi <= actual_cpi * 1.01:
        print("Correct CPI!")
        score += traceWeightage / 2
      elif actual_cpi * 0.95 <= student_cpi <= actual_cpi * 1.05:
        print("Partially correct CPI.")
        score += traceWeightage / 4
      else:
        print("Incorrect CPI.")
   
      if actual_mispred * 0.99 <= student_mispred <= actual_mispred * 1.01:
        print("Correct MISPRED_RATE!")
        score += traceWeightage / 2
      elif actual_mispred * 0.95 <= student_mispred <= actual_mispred * 1.05:
        print("Partially correct MISPRED_RATE.")
        score += traceWeightage / 4
      else:
        print("Incorrect MISPRED_RATE.")
      print()
        
    return score
  except subprocess.TimeoutExpired:
    sim.kill()
    exit()

scoreA1 = run_A("A1", ["-pipewidth", "1"])
weightedScoreA1 = round(scoreA1 * 2.0, 3)
print(f"Score: {weightedScoreA1} / 2.0\n")

scoreA2 = run_A("A2", ["-pipewidth", "2"])
weightedScoreA2 = round(scoreA2 * 2.0, 3)
print(f"Score: {weightedScoreA2} / 2.0\n")

scoreA3 = run_A("A3", ["-pipewidth", "2", "-enablememfwd", "-enableexefwd"])
weightageA3 = 3.0 if isUnderGrad else 2.0
weightedScoreA3 = round(scoreA3 * weightageA3, 3)
print(f"Score: {weightedScoreA3} / {weightageA3}\n")
  		
scoreB1 = run_B("B1", ["-pipewidth", "2", "-enablememfwd", "-enableexefwd", "-bpredpolicy", "1"])
weightageB1 = 2.0 if isUnderGrad else 1.0
weightedScoreB1 = round(scoreB1 * weightageB1, 3)
print(f"Score: {weightedScoreB1} / {weightageB1}\n")

scoreB2 = run_B("B2", ["-pipewidth", "2", "-enablememfwd", "-enableexefwd", "-bpredpolicy", "2"])
weightedScoreB2 = round(scoreB2 * 1.0, 3)
print(f"Score: {weightedScoreB2} / 1.0\n")

scoreB3 = run_B("B3", ["-pipewidth", "2", "-enablememfwd", "-enableexefwd", "-bpredpolicy", "3"])
weightedScoreB3 = round(scoreB3 * 1.0, 3)
print(f"Score: {weightedScoreB3} / {0.0 if isUnderGrad else 1.0}\n")

scoreB4 = run_B("B4", ["-pipewidth", "2", "-enablememfwd", "-enableexefwd", "-bpredpolicy", "4"])
weightedScoreB4 = round(scoreB4 * 1.0, 3)
print(f"Score: {weightedScoreB4} / {0.0 if isUnderGrad else 1.0}\n")

totalScore = weightedScoreA1 + weightedScoreA2 + weightedScoreA3 + weightedScoreB1 + weightedScoreB2 + weightedScoreB3 + weightedScoreB4
print(f"Total Score: {totalScore} / {10.0}")