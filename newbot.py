import torch
import sys
from transformers import GPT2LMHeadModel, GPT2Tokenizer

# Load the Fine-Tuned Model
fine_tuned_model = GPT2LMHeadModel.from_pretrained('./fine_tuned_gpt2_airline')
fine_tuned_tokenizer = GPT2Tokenizer.from_pretrained('./fine_tuned_gpt2_airline')


airline_service_prompt = (
    "Welcome to our Airline Services Chatbot! We're here to assist you with all your flight-related inquiries. "
    "Please provide us with your query, and we'll do our best to help you."
)
 
user_question = sys.argv[1]
full_prompt = airline_service_prompt + "\nUser: " + user_question+ "\nbot:"
input_ids = fine_tuned_tokenizer.encode(full_prompt, return_tensors='pt')
output = fine_tuned_model.generate(input_ids, max_new_tokens = 100, pad_token_id=fine_tuned_tokenizer.eos_token_id, no_repeat_ngram_size=3, num_return_sequences=1, top_p = 0.95, temperature = 0.1, do_sample = True)
gen_text = fine_tuned_tokenizer.decode(output[0], skip_special_tokens=True)
ans = ""
ans = gen_text.split("bot:")[1]
if("." in ans):
    ans = ans.split(".")[0]
if("\n" in ans):
    ans = " ".join(ans.split("\n"))

print("gpt2bot>"+ans)